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

#include "Router.h"
#include "../processing_element/flit_utils.h"

Router::Router(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName) {

	mRun = this->prepare();
	//this->init();
}

Router::~Router() {

}

void Router::init() {
	// Set or calculate other parameters ...
	mCurrentAddr = static_cast<uint16_t>(std::bitset<16>(
			mDealer.getModelParameter(mName, "address")).to_ulong());

	mRoutingBits = std::bitset<16>(
			mDealer.getModelParameter(mName, "routingBits"));
}

bool Router::prepare() {
	mSubscriber.setOwnershipName(mName);

	if (!mPublisher.bindSocket(mDealer.getPortNumFrom(mName))) {
		return false;
	}

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("simulation_model"),
			mDealer.getPortNumFrom("simulation_model"))) {
		return false;
	}

	for (auto depModel : mDealer.getModelDependencies()) {
		if (!mSubscriber.connectToPub(mDealer.getIPFrom(depModel),
				mDealer.getPortNumFrom(depModel))) {
			return false;
		}
	}

	mConnectivityBits = std::bitset<16>(
			mDealer.getModelParameter(mName, "connectivityBits"));

	// Subscriptions to events
	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");
	mSubscriber.subscribeTo("PacketGenerator");
	mSubscriber.subscribeTo("SimTimeChanged");

	if (mConnectivityBits[0]) {
		mSubscriber.subscribeTo("South");
		mSubscriber.subscribeTo("Credit_in_S++");

	}
	if (mConnectivityBits[1]) {
		mSubscriber.subscribeTo("West");
		mSubscriber.subscribeTo("Credit_in_W++");

	}
	if (mConnectivityBits[2]) {
		mSubscriber.subscribeTo("East");
		mSubscriber.subscribeTo("Credit_in_E++");
	}
	if (mConnectivityBits[3]) {
		mSubscriber.subscribeTo("North");
		mSubscriber.subscribeTo("Credit_in_N++");
	}

	// Synchronization
	if (!mSubscriber.prepareSubSynchronization(
			mDealer.getIPFrom("simulation_model"),
			mDealer.getSynchronizationPort())) {
		return false;
	}

	if (!mSubscriber.synchronizeSub()) {
		return false;
	}

	return true;
}

void Router::run() {

	while (mRun) {
		if (mSubscriber.receiveEvent()) {
			this->handleEvent();
		}
	}
}

void Router::handleEvent() {
	auto eventBuffer = mSubscriber.getEventBuffer();

	auto receivedEvent = event::GetEvent(eventBuffer);
	std::string eventName = receivedEvent->name()->str();
	mCurrentSimTime = receivedEvent->timestamp();
	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (receivedEvent->data_type() == event::EventData_Flit) {
		auto flitData = receivedEvent->data_as_Flit()->uint32();

		if (eventName == "PacketGenerator") {
			if (mFlits_RX_L.empty() || (mFlits_RX_L.size() < 3)) {
				mFlits_RX_L.push(flitData);
			} else {
				std::cout
						<< "\033[1;31mWarning: Local FIFO is full --> Event will be discarded.\033[0m"
						<< std::endl;
			}
		}
		// Flit comes from the South output of the previous router
		else if (eventName == "South") {
			if (mFlits_RX_N.empty() || (mFlits_RX_N.size() < 3)) {
				mFlits_RX_N.push(flitData);
			} else {
				std::cout
						<< "\033[1;31mWarning: North FIFO is full --> Event will be discarded.\033[0m"
						<< std::endl;
			}
		}
		// Flit comes from the West output of the previous router
		else if (eventName == "West") {
			if (mFlits_RX_E.empty() || (mFlits_RX_E.size() < 3)) {
				mFlits_RX_E.push(flitData);
			} else {
				std::cout
						<< "\033[1;31mWarning: East FIFO is full --> Event will be discarded.\033[0m"
						<< std::endl;
			}
		}
		// Flit comes from the North output of the previous router
		else if (eventName == "North") {
			if (mFlits_RX_S.empty() || (mFlits_RX_S.size() < 3)) {
				mFlits_RX_S.push(flitData);
			} else {
				std::cout
						<< "\033[1;31mWarning: South FIFO is full --> Event will be discarded.\033[0m"
						<< std::endl;
			}
		}
		// Flit comes from the East output of the previous router
		else if (eventName == "East") {
			if (mFlits_RX_W.empty() || (mFlits_RX_W.size() < 3)) {
				mFlits_RX_W.push(flitData);
			} else {
				std::cout
						<< "\033[1;31mWarning: West FIFO is full --> Event will be discarded.\033[0m"
						<< std::endl;
			}
		}

	} else if (receivedEvent->data_type() == event::EventData_String) {
		std::string configPath = receivedEvent->data_as_String()->str();

		if (eventName == "SaveState") {
			this->saveState(
					std::string(configPath.begin(), configPath.end()) + mName
							+ ".config");
		}

		else if (eventName == "LoadState") {
			this->loadState(
					std::string(configPath.begin(), configPath.end()) + mName
							+ ".config");
		}
	}

	else if (eventName == "SimTimeChanged") {
		// Send new Flit every clock cycle
		sendFlitWithRoundRobinPrioritization();
	}

	// Increase Credit Counter
	else if (eventName == "Credit_in_N++") {
		if (mCredit_Cnt_N < 3) {
			mCredit_Cnt_N++;
		}
	}

	else if (eventName == "Credit_in_W++") {
		if (mCredit_Cnt_W < 3) {
			mCredit_Cnt_W++;
		}
	}

	else if (eventName == "Credit_in_E++") {
		if (mCredit_Cnt_E < 3) {
			mCredit_Cnt_E++;
		}
	}

	else if (eventName == "Credit_in_S++") {
		if (mCredit_Cnt_S < 3) {
			mCredit_Cnt_S++;
		}
	}

	else if (eventName == "End") {
		mRun = false;
	}

}

void Router::sendFlitWithRoundRobinPrioritization() {
	// N->E->W->S->L (circular) prioritization the requests from inputs (from LBDR modules) are checked
	if (!sendFlitAfterRequestCheck(mReq_N_LBDR, mFlits_RX_N, "Credit_in_N++")) {
		if (!sendFlitAfterRequestCheck(mReq_E_LBDR, mFlits_RX_E,
				"Credit_in_E++")) {
			if (!sendFlitAfterRequestCheck(mReq_W_LBDR, mFlits_RX_W,
					"Credit_in_W++")) {
				if (!sendFlitAfterRequestCheck(mReq_S_LBDR, mFlits_RX_S,
						"Credit_in_S++")) {
					sendFlitAfterRequestCheck(mReq_L_LBDR, mFlits_RX_L,
							"Credit_in_L++");
				}
			}
		}
	}
}

bool Router::sendFlitAfterRequestCheck(Request& request,
		std::queue<uint32_t>& flitFIFO, std::string creditString) {

	bool sentFlit = true;

	if (!flitFIFO.empty()) {
		uint32_t flit = flitFIFO.front();
		uint32_t flitType = get_flit_type(flit);

		if (flitType == HEADER_FLIT) {
			generateRequest(flit, request);
		}

		if (request != Request::idle) {
			std::string reqString;

			// NORTH
			if ((request == Request::north) && (mCredit_Cnt_S > 0)) {
				reqString = "North";
				mCredit_Cnt_S--;
			}
			// EAST
			else if ((request == Request::east) && (mCredit_Cnt_W > 0)) {
				reqString = "East";
				mCredit_Cnt_W--;
			}
			// WEST
			else if ((request == Request::west) && (mCredit_Cnt_E > 0)) {
				reqString = "West";
				mCredit_Cnt_E--;
			}
			// SOUTH
			else if ((request == Request::south) && (mCredit_Cnt_N > 0)) {
				reqString = "South";
				mCredit_Cnt_N--;
			}
			// LOCAL
			else if ((request == Request::local)) {
				reqString = "Local";
			} else {
				sentFlit = false;
			}

			if (sentFlit) {
				this->sendFlit(flit, reqString);

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

					request = Request::idle;
				}

				flitFIFO.pop();
				this->updateCreditCounter(creditString);
			}
		} else {
			sentFlit = false;
		}
	}
	// Router is IDLE, because other routers block the outputs
	else {
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

void Router::sendFlit(uint32_t flit, std::string reqString) {
	std::cout << "\e[1mT=" << mCurrentSimTime << ": \e[0m" << mName << " sends " << flit << " to " << reqString
			<< " output" << std::endl;

	// Event Serialiazation
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	eventOffset = event::CreateEvent(fbb, fbb.CreateString(reqString),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(fbb, flit).Union());

	fbb.Finish(eventOffset);

	mPublisher.publishEvent(reqString, fbb.GetBufferPointer(),
			fbb.GetSize());
}

void Router::updateCreditCounter(std::string eventName) {
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	eventOffset = event::CreateEvent(fbb, fbb.CreateString(eventName),
			mCurrentSimTime);
	fbb.Finish(eventOffset);

	mPublisher.publishEvent(eventName, fbb.GetBufferPointer(),
			fbb.GetSize());
}

void Router::saveState(std::string filePath) {
// Store states
	std::ofstream ofs(filePath);
	boost::archive::xml_oarchive oa(ofs, boost::archive::no_header);
	try {
		oa << boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ": Archive Exception during serializing:"
				<< std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();
}

void Router::loadState(std::string filePath) {
// Restore states
	std::ifstream ifs(filePath);
	boost::archive::xml_iarchive ia(ifs, boost::archive::no_header);
	try {
		ia >> boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ": Archive Exception during deserializing:"
				<< std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();

	// Optional calculate parameters from the loaded initial state
	init();
}
