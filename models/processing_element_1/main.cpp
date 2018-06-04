/*
 * main.cpp
 *
 *  Created on: Jun 4, 2018
 *      Author: user
 */

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "resources/src/ProcessingElement.h"

int main() {
	ProcessingElement processingElement1("processing_element_1", "PE for Router 1: Packet Generator for Sending and Packet Sink for Receiving Flits");

	try {
		processingElement1.run();

	} catch (zmq::error_t& e) {
		std::cout << "PE 1: Interrupt received: Exit" << std::endl;
	}

	return 0;
}

