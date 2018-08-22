/*
 * main.cpp
 *
 *  Created on: Jun 4, 2018
 *      Author: user
 */

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "ProcessingElement.h"

int main(int argc, char* argv[]) {

	if (argc > 2) {
		bool validArgs = true;
		std::string peName = "";

		if (static_cast<std::string>(argv[1]) == "-n") {
			peName = static_cast<std::string>(argv[2]);
		} else {
			validArgs = false;
			std::cout << " Invalid argument/s: --help" << std::endl;
		}

		if (validArgs) {
			ProcessingElement processingElement(peName,
					"PE for Router: Packet Generator for Sending and Packet Sink for Receiving Flits");

			try {
				processingElement.run();

			} catch (zmq::error_t& e) {
				std::cout << peName<<" : Interrupt received: Exit" << std::endl;
			}
		}
	} else if (argc > 1) {
		if (static_cast<std::string>(argv[1]) == "--help") {
			std::cout << "<< Help >>" << std::endl;
			std::cout << "-n NAME >> "
					<< "Set instance name of Processing Element" << std::endl;
		} else {
			std::cout << " Invalid argument/s: --help" << std::endl;
		}
	} else {
		std::cout << " Invalid or missing argument/s: --help" << std::endl;
	}

	return 0;
}

