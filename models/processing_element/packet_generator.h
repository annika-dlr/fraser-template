/*
 * This file implements a simple packet generator with
 * fixed traffic (counter, no random data).
 */

#ifndef __PACKET_GENERATOR_HPP__
#define __PACKET_GENERATOR_HPP__

#include <cstdint>
#include <memory>
#include <queue>

enum class GenerationModes {counter}; // TODO: Add other modes

class PacketGenerator {
public:
    PacketGenerator(uint16_t address);
    void generate_packet(std::queue<uint32_t>& packet, uint16_t packet_length, uint16_t destination,
                         GenerationModes mode, uint64_t time);

    void set_local_address(uint16_t address) {
    	m_address = address;
    }

private:
    uint16_t counter_based_generation(std::queue<uint32_t>& packet,
                                      uint16_t packet_length, uint16_t destination);

    uint16_t m_address;
    uint16_t m_packet_id;
};


#endif //__PACKET_GENERATOR_HPP__
