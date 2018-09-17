/*
 * Copyright (c) 2018, German Aerospace Center (DLR)
 *
 * This file is part of the development version of FRASER.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 *  2018, Annika Ofenloch (DLR RYAVS)
 */

#ifndef FRASER_TEMPLATE_MODELS_ROUTER_ROUTER_H_
#define FRASER_TEMPLATE_MODELS_ROUTER_ROUTER_H_

#include <fstream>
#include <queue>
#include <string>
#include <stdint.h>
#include <bitset>
#include <iostream>

#include "flit_utils.h"

#define NOC_SIZE 2

class Router {
public:

	Router();

	enum Request
		: uint8_t {
			idle = 0, local = 1, north = 2, east = 3, south = 4, west = 5
	};

	bool arbitrateWithRoundRobinPrioritization();

	bool pushToLocalFIFO(uint32_t flit);
	bool pushToWestFIFO(uint32_t flit);
	bool pushToEastFIFO(uint32_t flit);
	bool pushToSouthFIFO(uint32_t flit);
	bool pushToNorthFIFO(uint32_t flit);

	void increaseCreditCntNorth();
	void increaseCreditCntWest();
	void increaseCreditCntEast();
	void increaseCreditCntSouth();

	// Getter & Setter
	void setAddress(uint16_t address) {
		mCurrentAddr = address;
	}

	void setRoutingBits(std::bitset<16> rountingBits) {
		mRoutingBits = rountingBits;
	}

	void setConnectivityBits(std::bitset<16> connectivityBits) {
		mConnectivityBits = connectivityBits;
	}

	uint32_t getNextFlit() {
		return mNextFlit;
	}

	std::string getChosenOutputPort() {
		return mChosenOutputPort;
	}

	std::string getCreditCntSignal() {
		return mCreditCntSignal;
	}

private:
	uint64_t mCycles = 0; // TODO: Forward Flit after 3 Cycles
	uint16_t mCurrentAddr = 0;
	std::bitset<16> mConnectivityBits = 0;
	std::bitset<16> mRoutingBits = 0;

	// Credit based flow
	uint16_t mCredit_Cnt_W = 3;
	uint16_t mCredit_Cnt_N = 3;
	uint16_t mCredit_Cnt_S = 3;
	uint16_t mCredit_Cnt_E = 3;

	std::queue<uint32_t> mFlits_RX_L; // L FIFO
	std::queue<uint32_t> mFlits_RX_N; // N FIFO
	std::queue<uint32_t> mFlits_RX_E; // E FIFO
	std::queue<uint32_t> mFlits_RX_S; // S FIFO
	std::queue<uint32_t> mFlits_RX_W; // W FIFO

	// Generated requests from LDBRs
	Request mReq_N_LBDR = idle;
	Request mReq_E_LBDR = idle;
	Request mReq_W_LBDR = idle;
	Request mReq_S_LBDR = idle;
	Request mReq_L_LBDR = idle;

	// Generated requests from LDBRs
	bool north_grant = false;
	bool east_grant = false;
	bool west_grant = false;
	bool south_grant = false;
	bool local_grant = false;

	uint32_t mNextFlit = 0;
	std::string mChosenOutputPort;
	std::string mCreditCntSignal;

	bool validGrantSignal(Request& request, std::queue<uint32_t>& flitFIFO,
			std::string creditString);

	// returns the request (Local, North, East, South or West)
	void generateRequest(uint32_t flit, Request& request);
};

#endif /* FRASER_TEMPLATE_MODELS_ROUTER_ROUTER_H_ */
