/*
 * Copyright (c) 2017, German Aerospace Center (DLR)
 *
 * This file is part of the development version of FRASER.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2017, Annika Ofenloch (DLR RY-AVS)
 */

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "Model_2.h"

int main() {
	Model2 model2("model_2", "Test Model 2");
	try {
		model2.run();

	} catch (zmq::error_t& e) {
		std::cout << "Model 2: Interrupt received: Exit" << std::endl;
	}

	return 0;
}

