/*
 * This file implements a simple packet generator with
 * fixed traffic (counter, no random data).
 */

#ifndef __PACKET_GENERATOR_H__
#define __PACKET_GENERATOR_H__

#include <cstdint>
#include <queue>
#include <boost/crc.hpp>

enum class GenerationModes {counter}; // TODO: Add other modes
enum class FlitType {header, firstBody, body, tail};
enum class GenerationStates {startupDelay, sendFlit, waitFrameEnd};

class PacketGenerator {
public:
    PacketGenerator() {
        mPacketId = 0;
    }
    
    void init(uint16_t address, uint8_t nocSize, GenerationModes generationMode, 
			    double pir, uint16_t minPacketLength, uint16_t maxPacketLength,
                uint64_t randomSeed, uint64_t generationEndTime);

    uint32_t getFlit(uint64_t time);

private:
    uint32_t counterBasedGeneration(uint64_t time);
    uint32_t generatePayload(uint64_t time);
    void printFlit(uint32_t flit, uint64_t time, uint8_t flitType, uint16_t dest);


    uint16_t mAddress;
	uint16_t mNocSize;

    /* Data storage for flit generation */
    uint16_t mPacketId;
    uint16_t mPacketLength;
    uint16_t mCounter;
    FlitType mFlitType;
    bool mWaiting;
    uint16_t mStartupDelay;
    uint16_t mDestination;
    boost::crc_ccitt_type mCrc;
    GenerationStates mGenerationState;

    /* User-definable constants, which need to be easily configurable by user */
    uint16_t mFrameLength;
    GenerationModes mGenerationMode;
    uint16_t mMinPacketLength;
	uint16_t mMaxPacketLength;
	uint64_t mRandomSeed;
	uint64_t mGenerationEndTime;
};


#endif //__PACKET_GENERATOR_H__
