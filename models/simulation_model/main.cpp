/*
 * main.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: Annika Ofenloch
 */

#include <iostream>
#include <string>

#include "SimulationModel.h"

int main(int argc, char* argv[]) {
	if (argc > 2) {
		std::string configFile = argv[2];
		SimulationModel simulation("simulation_model", "Simulation Environment");

		if (static_cast<std::string>(argv[1]) == "--create-config-files") {
			std::cout << "Create default configuration files" << std::endl;
			simulation.setConfigMode(true);
			simulation.store(configFile);

		} else if (static_cast<std::string>(argv[1]) == "--load-config") {
			simulation.configure(configFile);
			simulation.run();

		} else {
			std::cout << " Invalid argument/s: --help" << std::endl;
		}
	} else if (argc > 1) {
		if (static_cast<std::string>(argv[1]) == "--help") {
			std::cout << "<< Help >>" << std::endl;
			std::cout << "--create-config-files CONFIG-PATH >> "
					<< "Create default configuration files with initialized "
					<< "values and save them in CONFIG-PATH" << std::endl;
			std::cout
					<< "--load-config CONFIG-PATH >> Define path of configuration file/s"
					<< std::endl;
		} else {
			std::cout << " Invalid argument/s: --help" << std::endl;
		}
	} else {
		std::cout << " Invalid argument/s: --help" << std::endl;
	}

	return 0;
}
