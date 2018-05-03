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

#include "resources/src/Router.h"

int main() {
	Router router3("router_3", "Router 3");
	router3.setRouterAddr(0b00111111111010);

	try {
		router3.run();

	} catch (zmq::error_t& e) {
		std::cout << "Router 3: Interrupt received: Exit" << std::endl;
	}

	return 0;
}

