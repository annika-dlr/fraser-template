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
	ProcessingElement processingElement2("processing_element_2", "PE for Router 2: Packet Generator for Sending and Packet Sink for Receiving Flits");

	try {
		processingElement2.run();

	} catch (zmq::error_t& e) {
		std::cout << "PE 2: Interrupt received: Exit" << std::endl;
	}

	return 0;
}

