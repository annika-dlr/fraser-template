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
	ProcessingElement processingElement0("processing_element_0", "PE for Router 0: Packet Generator for Sending and Packet Sink for Receiving Flits");

	try {
		processingElement0.run();

	} catch (zmq::error_t& e) {
		std::cout << "PE 0: Interrupt received: Exit" << std::endl;
	}

	return 0;
}

