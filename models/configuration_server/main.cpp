/*
 * main.cpp
 *
 *  Created on: Dec 20, 2016
 *      Author: Annika Ofenloch
 */

#include "ConfigurationServer.h"

//  The main thread simply starts several clients and a server, and then
//  waits for the server to finish.

int main(int argc, char* argv[]) {
	if (argc > 2) {
		if (static_cast<std::string>(argv[1]) == "--models-config-file") {
			ConfigurationServer configServerModel(argv[2]);
			try {
				configServerModel.run();

			} catch (zmq::error_t& e) {
				std::cout << "ConfigurationServer: Interrupt received: Exit"
						<< std::endl;
			}
		} else {
			std::cout << " Invalid argument/s: --help" << std::endl;
		}
	} else if (argc > 1) {
		if (static_cast<std::string>(argv[1]) == "--help") {
			std::cout << "<< Help >>" << std::endl;
			std::cout << "--models-config-file CONFIG-PATH >> "
					<< "Set path of models-configuration file" << std::endl;
		} else {
			std::cout << " Invalid argument/s: --help" << std::endl;
		}
	} else {
		std::cout << " Invalid or missing argument/s: --help" << std::endl;
	}

	return 0;
}
