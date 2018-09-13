/*
 * A simple packet sink. It receives the flits, does correctness checking of the data
 * and logs the output.
 */

#include "packet_sink.h"

#include <iostream>
#include <string>
#include <sstream>
#include <bitset>
#include <boost/crc.hpp>

#include "flit_utils.h"

#define COLOR_RED       "\033[1;31m"
#define COLOR_GREEN     "\033[1;32m"
#define COLOR_YELLOW    "\033[1;33m"
#define COLOR_BLUE      "\033[1;34m"
#define COLOR_BOLD      "\e[1m"
#define COLOR_DEFAULT   "\033[0m"

/*
 * Initializes the packet sink.
 * 
 * Paramters:
 * 	uint16_t address: Address of the current node
 */
void PacketSink::init(uint16_t address) {
	mAddress = address;
	mCountedPacketLength = 0;
}

/*
 * Prints debug information about received flits.
 * 
 * Parameters:
 * 	uint32_t flit:     Received flit
 *  uint64_t time:     Current simulation time
 *  uint8_t flitType:  Type of the received flit
 */
void PacketSink::printFlit(uint32_t flit, uint64_t time, uint8_t flitType) {

	std::string flitTypeStr;
	std::string strColor;


	if (flitType == HEADER_FLIT) {
		flitTypeStr = "HEADER";
		strColor = "\033[1;33m";

	} else if (flitType == BODY_FLIT) {
		flitTypeStr = "BODY";
		strColor = "\033[1;34m";

	} else if (flitType == TAIL_FLIT) {
		flitTypeStr = "TAIL";
		strColor = "\033[1;31m";
	}

	std::cout << COLOR_BOLD << "T=" << time << ": " << COLOR_RED << "[R]" << strColor 
				<< "PE_" << mAddress << " [Received] " << flitTypeStr << " Flit " 
				<< COLOR_DEFAULT << flit << " ["<<std::bitset<32>(flit).to_string() << "]" 
				<< " from " << mRecvdPacket.srcAddr << " to " << mRecvdPacket.dstAddr << std::endl;
}

/*
 * Called if a flit with an unexpected type is received. It will put the system into a
 * erroneous state and log the flit as erroneous. This will also cause the FSM in
 * PacketSink::send_flit_to_local to drop all flits until a next header has arrived
 */
void PacketSink::fsmError(uint64_t time) {
	if (!mRecvError) {
		mRecvError = true;
		mNextState = PacketStates::waitHeader;
		std::cout << COLOR_BOLD << COLOR_RED << "[R][ERROR]" << COLOR_DEFAULT << " Node_" << mAddress 
			<< ": Wrong flit order detected!" << COLOR_BOLD 
			<< ", time: " << time << COLOR_DEFAULT << std::endl;
	}
}

/*
 * Logs the received packet.
 *
 * Parameters:
 *  bool fsmFault - Set to 'true' if flit order error is detected, otherwise 'false'
 *  uint32_t tailFlit - tailflit received from the network
 *  uint64_t time - Current simulation time
 */
void PacketSink::logPacket(uint32_t tailFlit, uint64_t time) {
	std::stringstream logStream;
	std::string statusColor;
	std::string statusStr;

	/* Extract CRC from tail */
	auto tailPayload = get_bit_range(tailFlit, 1, 28);
	uint16_t recvdCrc = tailPayload | 0xFFFF0000;
	
	/* Calculated CRC for comparison */
	auto calculatedCrc = mRecvdPacket.crc.checksum();

	bool faultyCrc = (calculatedCrc != recvdCrc);
	bool faultyLength = (mRecvdPacket.packetLength != mCountedPacketLength);

	if (faultyCrc || faultyLength) {
		statusColor = COLOR_RED;
		statusStr = " [FAULTY]";
	} else {
		statusColor = COLOR_GREEN;
		statusStr = " [OK]";
	}


	logStream << COLOR_BOLD << COLOR_RED << "[R]" << COLOR_BLUE << "[PACKET] " << COLOR_DEFAULT 
				<< "Node_" << mAddress << " - " 
				<< COLOR_BOLD << "Src: " << COLOR_DEFAULT << mRecvdPacket.srcAddr 
				<< COLOR_BOLD << ", Dst: " << COLOR_DEFAULT << mRecvdPacket.dstAddr 
				<< COLOR_BOLD << ", ID: " << COLOR_DEFAULT << mRecvdPacket.packetId 
				<< COLOR_BOLD << ", Encoded length: " << COLOR_DEFAULT << mRecvdPacket.packetLength 
				<< COLOR_BOLD << ", Counted length: " << COLOR_DEFAULT << mCountedPacketLength
				<< COLOR_BOLD << ", Encoded CRC: 0x" << COLOR_DEFAULT << std::hex << recvdCrc
				<< COLOR_BOLD << ", Calculated CRC: " << COLOR_DEFAULT << "0x" << std::hex << calculatedCrc << std::dec 
				<< COLOR_BOLD << ", time: " << COLOR_DEFAULT << time 
				<< COLOR_BOLD << statusColor << statusStr << COLOR_DEFAULT << std::endl;

	std::cout << logStream.str();
}

/*
 * Receives flit from the router and stores it in the memory. It also makes correctness checks.
 * It is called from the router.
 *
 * Parameters:
 *  uint32_t flit - flit to receive
 *  uint64_t time - current time
 */

void PacketSink::putFlit(uint32_t flit, uint64_t time) {

	uint8_t parity; // TODO not actually checked, but needed for receiving the flits
	auto flitType = get_flit_type(flit);

	mCountedPacketLength++;

	/* FSM for receiving the packet */
	switch (mNextState) {
	case PacketStates::waitHeader:

		mRecvdPacket.crc.reset();
		mCountedPacketLength = 1;
		if (flitType == HEADER_FLIT) {
			mRecvError = false;
			parse_header_flit(flit, &mRecvdPacket.dstAddr,
					&mRecvdPacket.srcAddr, &parity);
			mNextState = PacketStates::waitFirstBody;

		} else {
			fsmError(time);
		}

		break;

	case PacketStates::waitFirstBody:

		if (flitType == BODY_FLIT) {
			mRecvError = false;
			parse_first_body_flit(flit, &mRecvdPacket.packetLength,
					&mRecvdPacket.packetId, &parity);
			mNextState = PacketStates::waitTail;

		} else {
			fsmError(time);
		}

		break;

		/*
		 * In case of normal body / header, the actual payload is not interesting to us,
		 * the CRC will take care of it.
		 */
	case PacketStates::waitTail:

		if (flitType == BODY_FLIT) {
			mRecvError = false;
			mNextState = PacketStates::waitTail;
		}

		else if (flitType == TAIL_FLIT) {
			mRecvError = false;
			mNextState = PacketStates::waitHeader;

			logPacket(flit, time);
		} else {
			fsmError(time);
		}

		break;

	default:
		std::cout << COLOR_BOLD << COLOR_RED << "[R][ERROR]" << COLOR_DEFAULT << " Node_" << mAddress 
		<< ": Unknown PacketState!" << COLOR_BOLD 
		<< ", time: " << time << COLOR_DEFAULT << std::endl;
		return;
	}

	if (mNextState != PacketStates::waitHeader) {
		mRecvdPacket.crc.process_bytes(&flit, sizeof(uint32_t));
	}
	// printFlit(flit, time, flitType);


}
