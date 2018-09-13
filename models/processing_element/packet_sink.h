/*
 * A simple packet sink. It receives the flits, does correctness checking of the data
 * and logs the output.
 */

#ifndef __PACKET_SINK_H__
#define __PACKET_SINK_H__

#include <iostream>
#include <vector>
#include <cstdint>
#include <boost/crc.hpp>

enum class Faults {noFault, flitOrder, length, crc};
enum class PacketStates {waitHeader, waitFirstBody, waitTail};

struct Packet {
    uint16_t address;
    uint16_t srcAddr;
    uint16_t dstAddr;
    uint16_t packetLength;
    uint16_t packetId;
    boost::crc_ccitt_type crc;
};

class PacketSink {

public:
    PacketSink(){}
    void init(uint16_t address);
    void putFlit(uint32_t flit, uint64_t time);


private:
    void fsmError(uint64_t time);
    void logPacket(uint32_t tailFlit, uint64_t time);
    void printFlit(uint32_t flit, uint64_t time, uint8_t flitType);

    PacketStates mNextState = PacketStates::waitHeader;
    Packet mRecvdPacket;
    bool mRecvError = false;
    uint16_t mAddress;
    uint16_t mCountedPacketLength;

};

#endif //__PACKET_SINK_H__
