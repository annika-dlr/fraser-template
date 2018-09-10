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

#include "Router.h"

Router::Router() {

}

bool Router::arbitrateWithRoundRobinPrioritization() {
	// Router uses round-robin arbitration (with dynamic prioritization and fairness, avoiding starvation).
	// N->E->W->S->L (circular) prioritization the requests from inputs (from LBDR modules) are checked

	if (!validGrantSignal(mReq_N_LBDR, mFlits_RX_N, "Credit_in_N++")) {
		if (!validGrantSignal(mReq_E_LBDR, mFlits_RX_E, "Credit_in_E++")) {
			if (!validGrantSignal(mReq_W_LBDR, mFlits_RX_W, "Credit_in_W++")) {
				if (!validGrantSignal(mReq_S_LBDR, mFlits_RX_S,
						"Credit_in_S++")) {
					if (!validGrantSignal(mReq_L_LBDR, mFlits_RX_L,
							"Credit_in_L++")) {
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool Router::validGrantSignal(Request& request, std::queue<uint32_t>& flitFIFO,
		std::string creditString) {

	bool sentFlit = true;

	if (!flitFIFO.empty()) {
		uint32_t flit = flitFIFO.front();
		uint32_t flitType = get_flit_type(flit);

		if (flitType == HEADER_FLIT) {
			// LBDR generates the request
			generateRequest(flit, request);
		}

		if (request != Request::idle) {

			// NORTH
			if ((request == Request::north) && (mCredit_Cnt_S > 0)) {
				mChosenOutputPort = "North";
				mCredit_Cnt_S--;
			}
			// EAST
			else if ((request == Request::east) && (mCredit_Cnt_W > 0)) {
				mChosenOutputPort = "East";
				mCredit_Cnt_W--;
			}
			// WEST
			else if ((request == Request::west) && (mCredit_Cnt_E > 0)) {
				mChosenOutputPort = "West";
				mCredit_Cnt_E--;
			}
			// SOUTH
			else if ((request == Request::south) && (mCredit_Cnt_N > 0)) {
				mChosenOutputPort = "South";
				mCredit_Cnt_N--;
			}
			// LOCAL
			else if ((request == Request::local)) {
				mChosenOutputPort = "Local";
			} else {
				sentFlit = false;
			}

			if (sentFlit) {
				mNextFlit = flit;
				mCreditCntSignal = creditString;

				// Tail Flit
				if (flitType == TAIL_FLIT) {
					if (request == Request::north) {
						north_grant = false;
					} else if (request == Request::east) {
						east_grant = false;
					} else if (request == Request::west) {
						west_grant = false;
					} else if (request == Request::south) {
						south_grant = false;
					} else if (request == Request::local) {
						local_grant = false;
					}
				}

				flitFIFO.pop();
			}
		} else {
			// Router is IDLE, because other routers block the outputs
			sentFlit = false;
		}
	} else {
		// FIFO is empty
		sentFlit = false;
	}

	return sentFlit;
}

void Router::generateRequest(uint32_t flit, Request& request) {

	uint8_t parity = 0; // TODO not actually checked, but needed for receiving the flits
	uint16_t srcAddr = 0;
	uint16_t dstAddr = 0;
	bool n1 = false, e1 = false, w1 = false, s1 = false;
	parse_header_flit(flit, &dstAddr, &srcAddr, &parity);

	uint16_t des_addr_x = dstAddr % NOC_SIZE;
	uint16_t cur_addr_x = mCurrentAddr % NOC_SIZE;
	uint16_t des_addr_y = dstAddr / NOC_SIZE;
	uint16_t cur_addr_y = mCurrentAddr / NOC_SIZE;

	if (des_addr_y < cur_addr_y) {
		n1 = true;
	}
	if (cur_addr_x < des_addr_x) {
		e1 = true;
	}
	if (des_addr_x < cur_addr_x) {
		w1 = true;
	}
	if (cur_addr_y < des_addr_y) {
		s1 = true;
	}

	// Check if output is already used. If the output is not blocked (grant set to 1), a request is generated.
	if (((n1 && !e1 && !w1) || (n1 && e1 && mRoutingBits[0])
			|| (n1 && w1 && mRoutingBits[1])) && mConnectivityBits[0]
			&& (!north_grant)) {
		// Req_N
		request = Request::north;
		north_grant = true;
	} else if (((e1 && !n1 && !s1) || (e1 && n1 && mRoutingBits[2])
			|| (e1 && s1 && mRoutingBits[3])) && mConnectivityBits[1]
			&& (!east_grant)) {
		// Req_E
		request = Request::east;
		east_grant = true;
	} else if (((w1 && !n1 && !s1) || (w1 && n1 && mRoutingBits[4])
			|| (w1 && s1 && mRoutingBits[5])) && mConnectivityBits[2]
			&& (!west_grant)) {
		// Req_W
		request = Request::west;
		west_grant = true;
	} else if (((s1 && !e1 && !w1) || (s1 && e1 && mRoutingBits[6])
			|| (s1 && w1 && mRoutingBits[7])) && mConnectivityBits[3]
			&& (!south_grant)) {
		// Req_S
		request = Request::south;
		south_grant = true;
	} else if ((!n1 && !e1 && !w1 && !s1) && (!local_grant)) {
		// Req_L
		request = Request::local;
		local_grant = true;
	} else {
		request = Request::idle;
	}
}

void Router::increaseCreditCntNorth() {
	if (mCredit_Cnt_N < 3) {
		mCredit_Cnt_N++;
	}
}

void Router::increaseCreditCntWest() {
	if (mCredit_Cnt_W < 3) {
		mCredit_Cnt_W++;
	}
}

void Router::increaseCreditCntEast() {
	if (mCredit_Cnt_E < 3) {
		mCredit_Cnt_E++;
	}
}

void Router::increaseCreditCntSouth() {
	if (mCredit_Cnt_S < 3) {
		mCredit_Cnt_S++;
	}
}

bool Router::pushToLocalFIFO(uint32_t flit) {
	if (mFlits_RX_L.empty() || (mFlits_RX_L.size() < 3)) {
		mFlits_RX_L.push(flit);
		return true;
	}
	return false;
}

bool Router::pushToNorthFIFO(uint32_t flit) {
	if (mFlits_RX_N.empty() || (mFlits_RX_N.size() < 3)) {
		mFlits_RX_N.push(flit);
		return true;
	}
	return false;
}

bool Router::pushToEastFIFO(uint32_t flit) {
	if (mFlits_RX_E.empty() || (mFlits_RX_E.size() < 3)) {
		mFlits_RX_E.push(flit);
		return true;
	}
	return false;
}

bool Router::pushToWestFIFO(uint32_t flit) {
	if (mFlits_RX_W.empty() || (mFlits_RX_W.size() < 3)) {
		mFlits_RX_W.push(flit);
		return true;
	}
	return false;
}

bool Router::pushToSouthFIFO(uint32_t flit) {
	if (mFlits_RX_S.empty() || (mFlits_RX_S.size() < 3)) {
		mFlits_RX_S.push(flit);
		return true;
	}
	return false;
}

