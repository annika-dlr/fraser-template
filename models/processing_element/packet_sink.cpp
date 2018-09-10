/*
 * A simple packet sink. It receives the flits, does correctness checking of the data
 * and logs the output.
 */

#include "packet_sink.h"

#include <iostream>
#include <string>
#include <sstream>
#include <boost/crc.hpp>
#include <bitset>

#include "flit_utils.h"

/*
 * Initializes the packet sink.
 * 
 * Paramters:
 * 	uint16_t address: Address of the current node
 */
void PacketSink::init(uint16_t address) {
	mAddress = address;
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
	std::cout << "\e[1mT=" << time << ": " << "\033[1;31m[R] \033[0m" <<  strColor << "PE_" << mAddress 
				<< " [Received] " << flitTypeStr << " Flit \033[0m" << flit 
				<<" ["<<std::bitset<32>(flit).to_string() << "]" << " from "
				<< mRecvdPacket.srcAddr << " to " << mRecvdPacket.dstAddr << std::endl;
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
		logPacket(true, time);
	}
}

/*
 * Calculates the CRC over the packet
 *
 * Returns:
 *  CRC-CCITT-16 checksum
 */
uint16_t PacketSink::calculateCrc() {
	boost::crc_ccitt_type result;

	/* Calculate CRC of the packet (ignoring the tail flit) */
	// TODO: Add tail flit error checking
	result.process_bytes(&mRecvdPacket.packet[0],
			mRecvdPacket.packet.size() - 1);

	return result.checksum();
}

/*
 * Extracts CRC from the tail flit.
 *
 * Returns:
 *  The CRC extracted from the tail flit
 */
uint16_t PacketSink::extractCrc() {
	auto *tail_flit = &mRecvdPacket.packet[mRecvdPacket.packet.size() - 1];
	auto tail_payload = get_bit_range(*tail_flit, 1, 28);
	uint16_t recvd_crc = tail_payload | 0xFFFF0000;

	return recvd_crc;
}

/*
 * Logs the received packet.
 *
 * Parameters:
 *  bool faulty - True if the packet is marked as faulty
 */
void PacketSink::logPacket(bool faulty, uint64_t time) {
	std::stringstream logStream;

	if (faulty) {
		std::cout << "Recv_" << mAddress << ": Wrong flit order detected"
				<< std::endl;
	} else {

		auto extractedCrc = extractCrc();
		auto calculatedCrc = calculateCrc();

		logStream <<"\e[32m\e[1m[PACKET]\e[0m\e[39m Recv_" << mAddress << " - " << "ID: "
				<< mRecvdPacket.packetId << ", Src: "
				<< mRecvdPacket.srcAddr << ", Dst: "
				<< mRecvdPacket.dstAddr << ", Encoded length: "
				<< mRecvdPacket.packetLength << ", Counted length: "
				<< mRecvdPacket.packet.size() << ", Encoded CRC: 0x"
				<< std::hex << extractedCrc << ", Calculated CRC: 0x"
				<< std::hex << calculatedCrc << std::dec << ", time: " << time
				<< std::endl;

		std::cout << logStream.str();

		mRecvdPacket.packet.clear();
	}
}

/*
 * Receives flit from the router and stores it in the memory. It also makes correctness checks.
 * It is called from the router.
 *
 * Parameters:
 *  uint32_t flit - flit to receive
 */

void PacketSink::putFlit(uint32_t flit, uint64_t time) {

	uint8_t parity; // TODO not actually checked, but needed for receiving the flits
	auto flit_type = get_flit_type(flit);

	/* FSM for receiving the packet */
	switch (mNextState) {
	case PacketStates::waitHeader:

		if (flit_type == HEADER_FLIT) {
			mRecvError = false;
			parse_header_flit(flit, &mRecvdPacket.dstAddr,
					&mRecvdPacket.srcAddr, &parity);
			mRecvdPacket.packet.push_back(flit);
			mNextState = PacketStates::waitFirstBody;

		} else {
			fsmError(time);
		}

		break;

	case PacketStates::waitFirstBody:

		if (flit_type == BODY_FLIT) {
			mRecvError = false;
			parse_first_body_flit(flit, &mRecvdPacket.packetLength,
					&mRecvdPacket.packetId, &parity);
			mRecvdPacket.packet.push_back(flit);
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

		if (flit_type == BODY_FLIT) {
			mRecvError = false;
			mRecvdPacket.packet.push_back(flit);
			mNextState = PacketStates::waitTail;
		}

		else if (flit_type == TAIL_FLIT) {
			mRecvError = false;
			mRecvdPacket.packet.push_back(flit);
			mNextState = PacketStates::waitHeader;

			logPacket(false, time);

		} else {
			fsmError(time);
		}

		break;

	default:
		std::cerr << "Unknown PacketState!" << std::endl;
		return;
	}

	printFlit(flit, time, flit_type);


}
