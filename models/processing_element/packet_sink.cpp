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
 * Constructor
 *
 * Parameters:
 *  uint16_t address - Address of the current packet sink
 */
PacketSink::PacketSink(uint16_t address) {
	m_address = address;
}

/*
 * Called if a flit with an unexpected type is received. It will put the system into a
 * erroneous state and log the flit as erroneous. This will also cause the FSM in
 * PacketSink::send_flit_to_local to drop all flits until a next header has arrived
 */
void PacketSink::fsm_error(uint64_t time) {
	if (!m_recv_error) {
		m_recv_error = true;
		m_next_state = PacketStates::wait_header;
		log_packet(true, time);
	}
}

/*
 * Calculates the CRC over the packet
 *
 * Returns:
 *  CRC-CCITT-16 checksum
 */
uint16_t PacketSink::calculate_crc() {
	boost::crc_ccitt_type result;

	/* Calculate CRC of the packet (ignoring the tail flit) */
	// TODO: Add tail flit error checking
	result.process_bytes(&m_recvd_packet.packet[0],
			m_recvd_packet.packet.size() - 1);

	return result.checksum();
}

/*
 * Extracts CRC from the tail flit.
 *
 * Returns:
 *  The CRC extracted from the tail flit
 */
uint16_t PacketSink::extract_crc() {
	auto *tail_flit = &m_recvd_packet.packet[m_recvd_packet.packet.size() - 1];
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
void PacketSink::log_packet(bool faulty, uint64_t time) {
	std::stringstream log_stream;

	if (faulty) {
		std::cout << "Recv_" << m_address << ": Wrong flit order detected"
				<< std::endl;
	} else {

		auto extracted_crc = extract_crc();
		auto calculated_crc = calculate_crc();

		log_stream << "[PACKET] Recv_" << m_address << " - " << "ID: "
				<< m_recvd_packet.packet_id << ", Src: "
				<< m_recvd_packet.src_addr << ", Dst: "
				<< m_recvd_packet.dst_addr << ", Encoded length: "
				<< m_recvd_packet.packet_length << ", Counted length: "
				<< m_recvd_packet.packet.size() << ", Encoded CRC: 0x"
				<< std::hex << extracted_crc << ", Calculated CRC: 0x"
				<< std::hex << calculated_crc << std::dec << ", time: " << time
				<< std::endl;

		std::cout << log_stream.str();

		m_recvd_packet.packet.clear();
	}
}

/*
 * Receives flit from the router and stores it in the memory. It also makes correctness checks.
 * It is called from the router.
 *
 * Parameters:
 *  uint32_t flit - flit to receive
 */

void PacketSink::send_flit_to_local(uint32_t flit, uint64_t time) {

	uint8_t parity; // TODO not actually checked, but needed for receiving the flits
	auto flit_type = get_flit_type(flit);

	/* FSM for receiving the packet */
	switch (m_next_state) {
	case PacketStates::wait_header:

		if (flit_type == HEADER_FLIT) {
			m_recv_error = false;
			parse_header_flit(flit, &m_recvd_packet.dst_addr,
					&m_recvd_packet.src_addr, &parity);
			m_recvd_packet.packet.push_back(flit);
			m_next_state = PacketStates::wait_first_body;

			std::cout << "\033[1;33mSent HEADER Flit \033[0m" << flit <<" ["<<std::bitset<32>(flit).to_string()<<"]"<< " from "
					<< m_recvd_packet.src_addr << " to "
					<< m_recvd_packet.dst_addr << std::endl;

		} else {
			fsm_error(time);
		}

		break;

	case PacketStates::wait_first_body:

		if (flit_type == BODY_FLIT) {
			m_recv_error = false;
			parse_first_body_flit(flit, &m_recvd_packet.packet_length,
					&m_recvd_packet.packet_id, &parity);
			m_recvd_packet.packet.push_back(flit);
			m_next_state = PacketStates::wait_tail;

			std::cout << "\033[1;34mSent BODY Flit \033[0m" << flit<<" ["<<std::bitset<32>(flit).to_string()<<"]" << " from "
								<< m_recvd_packet.src_addr << " to "
								<< m_recvd_packet.dst_addr << std::endl;

		} else {
			fsm_error(time);
		}

		break;

		/*
		 * In case of normal body / header, the actual payload is not interesting to us,
		 * the CRC will take care of it.
		 */
	case PacketStates::wait_tail:

		if (flit_type == BODY_FLIT) {
			m_recv_error = false;
			m_recvd_packet.packet.push_back(flit);
			m_next_state = PacketStates::wait_tail;

			std::cout << "\033[1;34mSent BODY Flit \033[0m" << flit<<" ["<<std::bitset<32>(flit).to_string()<<"]" << " from "
					<< m_recvd_packet.src_addr << " to "
					<< m_recvd_packet.dst_addr << std::endl;

		}

		else if (flit_type == TAIL_FLIT) {
			m_recv_error = false;
			m_recvd_packet.packet.push_back(flit);
			m_next_state = PacketStates::wait_header;

			std::cout <<"\033[1;31mSent TAIL Flit \033[0m" << flit<<" ["<<std::bitset<32>(flit).to_string()<<"]" << " from "
					<< m_recvd_packet.src_addr << " to "
					<< m_recvd_packet.dst_addr << std::endl;

			log_packet(false, time);

		} else {
			fsm_error(time);
		}

		break;

	default:
		std::cerr << "Unknown PacketState!" << std::endl;
		return;
	}

}
