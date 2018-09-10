/*
 * A simple packet sink. It receives the flits, does correctness checking of the data
 * and logs the output.
 */

#ifndef __PACKET_SINK_H__
#define __PACKET_SINK_H__

#include <iostream>
#include <vector>
#include <cstdint>

enum class Faults {noFault, flitOrder, length, crc};
enum class PacketStates {waitHeader, waitFirstBody, waitTail};

struct Packet {
    uint16_t address;
    uint16_t srcAddr;
    uint16_t dstAddr;
    uint16_t packetLength;
    uint16_t packetId;
    uint16_t crc;

    std::vector<uint32_t> packet;
};

class PacketSink {

public:
    PacketSink(){}
    void init(uint16_t address);
    void putFlit(uint32_t flit, uint64_t time);


private:
    void fsmError(uint64_t time);
    void logPacket(bool faulty, uint64_t time);
    uint16_t extractCrc();
    uint16_t calculateCrc();
    void printFlit(uint32_t flit, uint64_t time, uint8_t flitType);

    uint16_t mAddress;
    Packet mRecvdPacket;
    PacketStates mNextState = PacketStates::waitHeader;
    bool mRecvError = false;
};

#endif //__PACKET_SINK_H__
