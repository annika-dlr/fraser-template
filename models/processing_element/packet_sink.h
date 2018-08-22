/*
 * A simple packet sink. It receives the flits, does correctness checking of the data
 * and logs the output.
 */

#ifndef __PACKET_SINK_HPP__
#define __PACKET_SINK_HPP__

#include <iostream>
#include <vector>
#include <cstdint>

enum class Faults {no_fault, flit_order, length, crc};
enum class PacketStates {wait_header, wait_first_body, wait_tail};

struct Packet {
    uint16_t address;
    uint16_t src_addr;
    uint16_t dst_addr;
    uint16_t packet_length;
    uint16_t packet_id;
    uint16_t crc;

    std::vector<uint32_t> packet;
};

class PacketSink {

public:
    PacketSink(uint16_t address);
    void send_flit_to_local(uint32_t flit);

private:
    void fsm_error();
    void log_packet(bool faulty=false);
    uint16_t extract_crc();
    uint16_t calculate_crc();

    uint16_t m_address;
    Packet m_recvd_packet;
    PacketStates m_next_state = PacketStates::wait_header;
    bool m_recv_error = false;
};

#endif //__PACKET_SINK_HPP__
