/*
 * Copyright (c) 2017-2018, German Aerospace Center (DLR)
 *
 * This file is part of the development version of FRASER.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2018, Annika Ofenloch (DLR RY-AVS)
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

