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
//#include "router.hpp"

/*
 * Constructor
 *
 * Parameters:
 * 	uint16_t address               - Address of the current packet generator
 * 	std::shared_ptr<Router> router - Pointer to the router (input port) instance
 * 					                 the packet generator is connected to
 */
PacketGenerator::PacketGenerator(uint16_t address = 1/*,std::shared_ptr<Router> router*/) {
	m_address = address;
	m_packet_id = 0;
	//m_router = router;
}

/*
 * Generates a packet by increasing the value stored in each flit by 1, starting from 0.
 *
 * Parameters:
 * 	std::shared_ptr<std::vector<uint32_t>> packet - A vector used for storing the packet flit-by-flit
 * 	uint16_t packet_length                        - Length of the packet in flits
 * 	uint16_t destination                          - Destination address for the packet
 */
uint16_t PacketGenerator::counter_based_generation(std::vector<uint32_t>& packet,
													uint16_t packet_length, uint16_t destination) {
	boost::crc_ccitt_type result;

	/* Build headers */
	packet.push_back(make_header_flit(destination, m_address));
	packet.push_back(make_first_body_flit(packet_length, m_packet_id));

	/* Make data flits */
	for (size_t i=2; i < packet_length - 1; i++){
		packet.push_back(make_body_flit(i));
	}
	/* Calculate result of the packet (ignoring the tail flit) */
    result.process_bytes(&packet.at(0), packet.size()); // TODO: include tail flit

	auto checksum = result.checksum();

	/* Store checksum in the tail */
	packet.push_back(make_tail_flit(checksum));

	return checksum;
}


/*
 * A simple packet generator. Currently it only implements counter-based data
 * generation, no random data support.
 *
 * Parameters:
 * 	uint16_t packet_length - Length of the data to be generated
 * 							 (in packets, including headers and tail)
 * 	uint16_t destination   - Destination address of the packet
 * 	GenerationModes mode   - Mode to use for data generation
 */
void PacketGenerator::generate_packet(std::vector<uint32_t>& packet, uint16_t packet_length, uint16_t destination,
										GenerationModes mode, uint64_t time) {
	std::stringstream log_stream;

	// std::vector<uint32_t> packet;
	// std::shared_ptr<std::vector<uint32_t>> packet = std::make_shared<std::vector<uint32_t>>();
	uint16_t checksum;

	/* Packet generation */
	// TODO: Add more modes (like random)
	switch (mode) {
		case GenerationModes::counter:
			checksum = counter_based_generation(packet, packet_length, destination);
			break;

		default:
			std::cerr << "Unknown data generation mode!" << std::endl;
			return;
	}

	/* Send the packet */
	// for (size_t i=0; i < packet->size(); i++) {
	// 	//m_router->send_flit_to_router(packet->at(i));
	// }

	/* Packet ID will increase after every sent packet */
	m_packet_id++;

	/* Logging */
	log_stream << "Sent_" << m_address << " - "
		<< "ID: " << m_packet_id
		<< ", Dst: " << destination
		<< ", Length: " << packet_length
		<< ", CRC: 0x" << std::hex << checksum
		<< std::dec << ", time: " << time
		<< std::endl;

	// TODO: Better logging than just printing on the screen??
    std::string log_line  = log_stream.str();
	std::cout << log_line;
}
