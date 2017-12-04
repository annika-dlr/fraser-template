/*
 * main.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: Annika Ofenloch
 */

#include <iostream>
#include <string>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "SimulationModel.h"

SimulationModel simulation("simulation_model", "Simulation Environment");
std::string configFile = "";

void startSimulationThread() {
	try {
		simulation.configure(configFile);
		simulation.run();
	} catch (boost::thread_interrupted&) {
		std::cout << " SimulationModel: Interrupt received: Exit" << std::endl;
	}
}

void createConfigurationFilesThread() {
	try {
		simulation.setConfigMode(true);
		simulation.store(configFile);
	} catch (boost::thread_interrupted&) {
		std::cout << " SimulationModel: Interrupt received: Exit" << std::endl;
	}
}

int main(int argc, char* argv[]) {
	if (argc > 2) {
		configFile = argv[2];
		if (static_cast<std::string>(argv[1]) == "--create-config-files") {
			std::cout
					<< "Configuration Mode: Create default configuration files"
					<< std::endl;

			boost::thread configThread(createConfigurationFilesThread);
			configThread.join();
		} else if (static_cast<std::string>(argv[1]) == "--load-config") {
			std::cout << "--------> Create simulation thread " << std::endl;
			boost::thread simThread(startSimulationThread);
			simThread.join();   // main thread waits for the thread t to finish
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
