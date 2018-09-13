/*
 * This file implements a simple packet generator with
 * fixed traffic (counter, no random data).
 */

#include "packet_generator.h"

#include <iostream>
#include <string>
#include <sstream>
#include <bitset>
#include <cstdint>
#include <memory>
#include <boost/crc.hpp>

#include "flit_utils.h"

#define COLOR_RED       "\033[1;31m"
#define COLOR_GREEN     "\033[1;32m"
#define COLOR_YELLOW    "\033[1;33m"
#define COLOR_BLUE      "\033[1;34m"
#define COLOR_BOLD      "\e[1m"
#define COLOR_DEFAULT   "\033[0m"

/*
 * Initializes the packet generator
 * 
 * Parameters:
 * 	uint16_t address 				- address of the current node
 * 	uint8_t nocSize					- Number of node in the NoC
 * 	GenerationModes generationMode	- packet generation mode
 * 	double pir						- Packet Injection Rate (a value between 0 and 1), packets per (clock)cycle
 * 	uint16_t minPacketLength		- Minimum packet length (Should be >= 3)
 * 	uint16_t maxPacketLength		- Maximum packet length (Should be <= 1/pir)
 *  uint64_t randomSeed				- Seed for the random generator
 *  uint64_t generationEndTime		- Time when to stop sending the packets
 */
void PacketGenerator::init(uint16_t address, uint8_t nocSize, GenerationModes generationMode, 
							double pir, uint16_t minPacketLength, uint16_t maxPacketLength,
							uint64_t randomSeed, uint64_t generationEndTime){
	mAddress = address;
	mGenerationMode = generationMode;
	mRandomSeed = randomSeed;
	mGenerationEndTime = generationEndTime;

	if ((pir < 0 || pir > 1)) {
		std::cout << COLOR_BOLD << COLOR_RED << "WARNING:" << COLOR_DEFAULT << " PIR value (" << pir
		<< ") is not between 0 and 1... auto-assuming PIR = 0.01" << std::endl;
		mFrameLength = 1/0.01;
		
	} else if (pir == 0) {
		std::cout << COLOR_BOLD << COLOR_RED << "WARNING:" << COLOR_DEFAULT << " PIR = 0, no traffic will be generated" << std::endl;
		mFrameLength = 0;
	    
	} else {
		mFrameLength = 1/pir;
	}

	if (minPacketLength > maxPacketLength) {
		std::cout << COLOR_BOLD << COLOR_RED << "WARNING:" << COLOR_DEFAULT << " minPacketLength (" << minPacketLength
		<< ") is larger than maxPacketLength (" << maxPacketLength
		<< ")... swapping the MIN and MAX values";

		mMinPacketLength = maxPacketLength;
		mMaxPacketLength = minPacketLength;

	} else {
		mMaxPacketLength = maxPacketLength;
		mMinPacketLength = minPacketLength;
	}

	if (mMinPacketLength < 3) {
		std::cout << COLOR_BOLD << COLOR_RED << "WARNING:" << COLOR_DEFAULT << " minPacketLength is too small (" << mMinPacketLength
		<< ")... auto-assuming minPacketLength = 3" << std::endl;

		mMinPacketLength = 3;
	}

	if (mMaxPacketLength > mFrameLength) {
		std::cout << COLOR_BOLD << COLOR_RED << "WARNING:" << COLOR_DEFAULT << " maxPacketLength (" << mMaxPacketLength
		<< ") is larger than the frame length (" << mFrameLength
		<< ")... auto-assuming maxPacketLength = frameLength" << std::endl;

		mMaxPacketLength = mFrameLength;
	}

	std::cout << "Node_" << address << ": Generating with PIR " << pir
		<< " (Frame Length " << mFrameLength << "), packet length between "
		<< minPacketLength << " and " << maxPacketLength << std::endl;

	mCounter = 0;
	mFlitType = FlitType::header;
	mWaiting = true;
	mGenerationState = GenerationStates::startupDelay;
	mStartupDelay = 3; // TODO: Replace with random
	mPacketLength = 10; // TODO: Replace with random
}

/*
 * Prints debug information about sent flits.
 * 
 * Parameters:
 * 	uint32_t flit:     Sent flit
 *  uint64_t time:     Current simulation time
 *  uint8_t flitType:  Type of the sent flit
 *  uint16_t dest:     Destination of the flit
 */
void PacketGenerator::printFlit(uint32_t flit, uint64_t time, uint8_t flitType, uint16_t dest) {

	std::string flitTypeStr;
	std::string strColor;


	if (flitType == HEADER_FLIT) {
		flitTypeStr = "HEADER";
		strColor = COLOR_YELLOW;

	} else if (flitType == BODY_FLIT) {
		flitTypeStr = "BODY";
		strColor = COLOR_BLUE;

	} else if (flitType == TAIL_FLIT) {
		flitTypeStr = "TAIL";
		strColor = COLOR_RED;
	}
	std::cout << COLOR_BOLD << "T=" << time << ": " << COLOR_GREEN << "[S] " << strColor 
					<< "PE_" << mAddress << " [Sent] " << flitTypeStr << " Flit " << COLOR_DEFAULT 
					<< flit <<" ["<<std::bitset<32>(flit).to_string() << "]" 
					<< " to " << dest << std::endl;
}

/*
 * Implements a flit generation with different generation modes.
 *
 * Parameters:
 * 	uint64_t time - current simulation time
 * 
 * Returns:
 * 	uint32_t Generated payload, 0 when generation failed
 */
uint32_t PacketGenerator::generatePayload(uint64_t time) {

	uint32_t payload;

	/* Packet generation */
	// TODO: Add more modes (like random)
	switch (mGenerationMode) {
		case GenerationModes::counter:
			payload = mCounter + 1;
			break;

		default:
			std::cout << COLOR_BOLD << COLOR_RED << "[S][ERROR]" << COLOR_DEFAULT << " Node_" << mAddress 
				<< ": Unknown generation mode!" << COLOR_BOLD 
				<< ", time: " << time << COLOR_DEFAULT << std::endl;
			return 0;
		}

	return payload;

}

/*
 * Gets a flit from packet generator.
 * 
 * Parameters:
 * 	uint64_t time - current simulation time
 * 
 * Returns:
 * 	uint32_t flit: The generated flit. If no flit is generated, 0 is returned.
 */

uint32_t PacketGenerator::getFlit(uint64_t time){
	std::stringstream logStream;
	auto flitNum = mCounter - mStartupDelay + 1;
	std::string logLine;
	uint32_t flit;

	mCounter++;

	switch (mGenerationState) {
		case GenerationStates::startupDelay:

			mCrc.reset();

			if (mCounter == mStartupDelay && time < mGenerationEndTime) {
				mGenerationState = GenerationStates::sendFlit;
			}
			break;
		
		case GenerationStates::sendFlit:

			switch (mFlitType) {
				case FlitType::header:
					{
						mDestination = 1; // TODO: Implement random destination generation
						if (mDestination == mAddress) {
							mDestination = 2;
						}

						flit = make_header_flit(mDestination, mAddress);
						mFlitType = FlitType::firstBody;
					}
					break;
				
				case FlitType::firstBody:
					{
						flit = make_first_body_flit(mPacketLength, mPacketId);
						mFlitType = FlitType::body;
					}
					break;

				case FlitType::body:
					{
						flit = make_body_flit(mCounter);

						if (flitNum == mPacketLength - 1) {
							mFlitType = FlitType::tail;
						} else {
							mFlitType = FlitType::body;
						}
					}
					break;

				case FlitType::tail:
					{
						auto checksum = mCrc.checksum();
						flit = make_tail_flit(checksum);

						mFlitType = FlitType::header;

						/* When we find the tail, let's log the sent packet */
						logStream << COLOR_GREEN << "[S]" << COLOR_BLUE << "[PACKET] " 
									<< COLOR_DEFAULT << "Node_" << mAddress << " - " 
									<< COLOR_BOLD << "Dst: " << COLOR_DEFAULT << mDestination 
									<< COLOR_BOLD << ", ID: " << COLOR_DEFAULT << mPacketId
									<< COLOR_BOLD << ", Length: " << COLOR_DEFAULT << mPacketLength
									<< COLOR_BOLD << ", Counted length: " << COLOR_DEFAULT << flitNum
									<< COLOR_BOLD << ", CRC:" << COLOR_DEFAULT << " 0x" << std::hex << checksum << std::dec
									<< COLOR_BOLD << ", time: " << COLOR_DEFAULT << time << std::endl;

						logLine = logStream.str();
						std::cout << logLine;
					}
					break;

				default:
						std::cout << COLOR_BOLD << COLOR_RED << "[S][ERROR]" << COLOR_DEFAULT << " Node_" << mAddress 
									<< ": Unknown Flit type!" << COLOR_BOLD 
									<< ", time: " << time << COLOR_DEFAULT << std::endl;
					return 0;
			}

			if (flitNum == mPacketLength) {
				mGenerationState = GenerationStates::waitFrameEnd;
			}

			mCrc.process_bytes(&flit, sizeof(uint32_t));
			printFlit(flit, time, get_flit_type(flit), mDestination);

			break;


		case GenerationStates::waitFrameEnd:
			if (mCounter == mFrameLength) {
				mStartupDelay = 2; // TODO: replace with random
				mPacketLength = 10; // TODO: Replace with random
				mCounter = 0;
				mGenerationState = GenerationStates::startupDelay;
				mPacketId++;
			}
			break;

		default:
			std::cout << COLOR_BOLD << COLOR_RED << "[S][ERROR]" << COLOR_DEFAULT << " Node_" << mAddress 
						<< ": Unknown Frame Generation State!" << COLOR_BOLD 
						<< ", time: " << time << COLOR_DEFAULT << std::endl;
			return 0;
	}

	
	return flit;
}
