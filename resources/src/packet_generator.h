/*
 * This file implements a simple packet generator with
 * fixed traffic (counter, no random data).
 */

#ifndef __PACKET_GENERATOR_HPP__
#define __PACKET_GENERATOR_HPP__

#include <cstdint>
#include <memory>
#include <vector>

//#include "router.hpp"

enum class GenerationModes {counter}; // TODO: Add other modes

class PacketGenerator {
public:
    PacketGenerator(uint16_t address/*, std::shared_ptr<Router> router*/);
    void generate_packet(uint16_t packet_length, uint16_t destination,
                         GenerationModes mode);

private:
    uint16_t counter_based_generation(std::shared_ptr<std::vector<uint32_t>> packet,
                                      uint16_t packet_length, uint16_t destination);

    uint16_t m_address;
    uint16_t m_packet_id;
    //std::shared_ptr<Router> m_router;
};


#endif //__PACKET_GENERATOR_HPP__
