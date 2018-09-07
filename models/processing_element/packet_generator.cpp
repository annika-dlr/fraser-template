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
 */
void PacketGenerator::init(uint16_t address, uint8_t nocSize, GenerationModes generationMode, 
							double pir, uint16_t minPacketLength, uint16_t maxPacketLength){
	mAddress = address;
	mGenerationMode = generationMode;

	if ((pir < 0 || pir > 1)) {
		std::cout << "WARNING: PIR value (" << pir \
		<< ") is not between 0 and 1... auto-assuming PIR = 0.01" << std::endl;
		mFrameLength = 1/0.01;
		
	} else if (pir == 0) {
		std::cout << "WARNING: PIR = 0, no traffic will be generated" << std::endl;
		mFrameLength = 0;
	    
	} else {
		mFrameLength = 1/pir;
	}

	if (minPacketLength > maxPacketLength) {
		std::cout << "WARNING: minPacketLength (" << minPacketLength \
		<< ") is larger than maxPacketLength (" << maxPacketLength \
		<< ")... swapping the MIN and MAX values";

		mMinPacketLength = maxPacketLength;
		mMaxPacketLength = minPacketLength;

	} else {
		mMaxPacketLength = maxPacketLength;
		mMinPacketLength = minPacketLength;
	}

	if (mMinPacketLength < 3) {
		std::cout << "WARNING: minPacketLength is too small (" << mMinPacketLength \
		<< ")... auto-assuming minPacketLength = 3" << std::endl;

		mMinPacketLength = 3;
	}

	if (mMaxPacketLength > mFrameLength) {
		std::cout << "WARNING: maxPacketLength (" << mMaxPacketLength \
		<< ") is larger than the frame length (" << mFrameLength \
		<< ")... auto-assuming maxPacketLength = frameLength" << std::endl;

		mMaxPacketLength = mFrameLength;
	}

	std::cout << "Node_" << address << ": Generating with PIR " << pir \
		<< " (Frame Length " << mFrameLength << "), packet length between " \
		<< minPacketLength << " and " << maxPacketLength << std::endl;

	mCounter = 0;
	mFlitType = FlitType::header;
	mWaiting = true;
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
			std::cerr << "Unknown generation mode!" << std::endl;
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

	std::stringstream log_stream;
	std::string log_line;
	uint32_t flit;
	uint32_t returnValue;

	if (mWaiting) {
		if(mCounter == 0) { // Start of a new frame
			mStartupDelay = 0; // TODO: Temporary placeholder. Should be replaced with a random value
			mPacketLength = mMaxPacketLength; // TODO: Implement random packet length
		}

		if (mStartupDelay == mCounter) {
			mWaiting = false;
		}
	}

	if (!mWaiting) {
		switch (mFlitType) {
			case FlitType::header:
				{
					mDestination = 1; // TODO: Implement random destination generation
					if (mDestination == mAddress) {
						mDestination++;
					}

					flit = make_header_flit(mDestination, mAddress);
					mFlitType = FlitType::firstBody;
				}
				break;
			
			case FlitType::firstBody:
				{
					// result.process_bytes(&packet.front(), packet.size());
					flit = make_first_body_flit(mPacketLength, mPacketId);
					mFlitType = FlitType::body;
				}
				break;

			case FlitType::body:
				{
					flit = make_body_flit(mCounter);

					if (mCounter - mStartupDelay > mPacketLength - 1) {
						mFlitType = FlitType::body;
					} else {
						mFlitType = FlitType::tail;
					}
				}
				break;

			case FlitType::tail:
				{
					// auto checksum = result.checksum();
					auto checksum = 42; // FIXME: CRC calculation is currently broken
					flit = make_tail_flit(checksum);

					mFlitType = FlitType::header;

					/* When we find the tail, let's log the sent packet */
					log_stream << "[PACKET] Sent_" << mAddress << " - " << "ID: " << mPacketId
								<< ", Dst: " << mDestination << ", Length: " << mPacketLength
								// << ", CRC: 0x" << std::hex << checksum << std::dec << ", time: "
								<< time << std::endl;

					// TODO: Better logging than just printing on the screen??
					log_line = log_stream.str();
					std::cout << log_line;
				}
				break;

			default:
				std::cerr << "Unknown Flit type!" << std::endl;
				return 0;
		}
	}

	if (mCounter == mFrameLength - 1) { // End of the frame
		mCounter = 0;
		mPacketId++;

	} else {
		mCounter++;
	}

	if (!mWaiting && mFlitType == FlitType::header) { // We had a tail flit
		returnValue = flit;
		mWaiting = true;

	} else if (mWaiting) {
		returnValue = 0;

	} else {
		returnValue = flit;
	}
	
	return returnValue;
}
