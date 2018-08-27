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
				mCtx), mDealer(mCtx, mName), mCurrentSimTime(0) {

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

	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");
	mSubscriber.subscribeTo("Local");
	mSubscriber.subscribeTo("SimTimeChanged");

	if (mConnectivityBits[0]) {
		mSubscriber.subscribeTo("North");
		mSubscriber.subscribeTo("Credit_in_S++");

	}
	if (mConnectivityBits[1]) {
		mSubscriber.subscribeTo("East");
		mSubscriber.subscribeTo("Credit_in_W++");

	}
	if (mConnectivityBits[2]) {
		mSubscriber.subscribeTo("West");
		mSubscriber.subscribeTo("Credit_in_E++");
	}
	if (mConnectivityBits[3]) {
		mSubscriber.subscribeTo("South");
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

		if (eventName == "Local") {
//			if (mFlits_RX_L.empty()) {
//				mFlits_RX_L.push(flitData);
//			} else if (mFlits_RX_L.size() < 3) {
			mFlits_RX_L.push(flitData);
//			} else {
//				std::cout
//						<< "\033[1;31m!!!!!!!!!!!! LOST EVENT !!!!!!!!!!!!!!!\033[0m"
//						<< std::endl;
//			}
		}

		else if (eventName == "North") {
//			if (mFlits_RX_N.empty()) {
//				mFlits_RX_N.push(flitData);
//			} else if (mFlits_RX_N.size() < 3) {
			mFlits_RX_N.push(flitData);
//			} else {
//				std::cout
//						<< "\033[1;31m!!!!!!!!!!!! LOST EVENT !!!!!!!!!!!!!!!\033[0m"
//						<< std::endl;
//			}
		}

		else if (eventName == "East") {
//			if (mFlits_RX_E.empty()) {
//				mFlits_RX_E.push(flitData);
//			} else if (mFlits_RX_E.size() < 3) {
			mFlits_RX_E.push(flitData);
//			} else {
//				std::cout
//						<< "\033[1;31m!!!!!!!!!!!! LOST EVENT !!!!!!!!!!!!!!!\033[0m"
//						<< std::endl;
//			}
		}

		else if (eventName == "South") {
//			if (mFlits_RX_S.empty()) {
//				mFlits_RX_S.push(flitData);
//			} else if (mFlits_RX_S.size() < 3) {
			mFlits_RX_S.push(flitData);
//			} else {
//				std::cout
//						<< "\033[1;31m!!!!!!!!!!!! LOST EVENT !!!!!!!!!!!!!!!\033[0m"
//						<< std::endl;
//			}
		}

		else if (eventName == "West") {
//			if (mFlits_RX_W.empty()) {
//				mFlits_RX_W.push(flitData);
//			} else if (mFlits_RX_W.size() < 3) {
			mFlits_RX_W.push(flitData);
//			} else {
//				std::cout
//						<< "\033[1;31m!!!!!!!!!!!! LOST EVENT !!!!!!!!!!!!!!!\033[0m"
//						<< std::endl;
//			}
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
		sendFlitWithRoundRobinPrioritization();
	}

	// Increase Credit Counter
	else if (eventName == "Credit_in_N++") {
		if (mCredit_Cnt_N < 3) {
			mCredit_Cnt_N++;
		}
//		std::cout << mName << " mCredit_Cnt_N++: " << mCredit_Cnt_N
//				<< std::endl;
	}

	else if (eventName == "Credit_in_W++") {
		if (mCredit_Cnt_W < 3) {
			mCredit_Cnt_W++;
		}
//		std::cout << mName << " mCredit_Cnt_W++: " << mCredit_Cnt_W
//				<< std::endl;
	}

	else if (eventName == "Credit_in_E++") {
		if (mCredit_Cnt_E < 3) {
			mCredit_Cnt_E++;
		}
//		std::cout << mName << " mCredit_Cnt_E++: " << mCredit_Cnt_E
//				<< std::endl;
	}

	else if (eventName == "Credit_in_S++") {
		if (mCredit_Cnt_S < 3) {
			mCredit_Cnt_S++;
		}
//		std::cout << mName << " mCredit_Cnt_S++: " << mCredit_Cnt_S
//				<< std::endl;
	}

	else if (eventName == "End") {
		mRun = false;
	}

}

void Router::sendFlitWithRoundRobinPrioritization() {
// N->E->W->S->L (circular) prioritization the requests from inputs (from LBDR modules) are checked
	if (!sendFlitAfterRequestCheck(mReq_N_LBDR, mFlits_RX_N, "Credit_in_N++")) {
//		std::cout<<mName<<": Get Flit from North FIFO"<<std::endl;
		if (!sendFlitAfterRequestCheck(mReq_E_LBDR, mFlits_RX_E,
				"Credit_in_E++")) {
//			std::cout<<mName<<": Get Flit from East FIFO"<<std::endl;
			if (!sendFlitAfterRequestCheck(mReq_W_LBDR, mFlits_RX_W,
					"Credit_in_W++")) {
//				std::cout<<mName<<": Get Flit from West FIFO"<<std::endl;
				if (!sendFlitAfterRequestCheck(mReq_S_LBDR, mFlits_RX_S,
						"Credit_in_S++")) {
//					std::cout<<mName<<": Get Flit from Local FIFO"<<std::endl;
					sendFlitAfterRequestCheck(mReq_L_LBDR, mFlits_RX_L,
							"Credit_in_L++");
				}
			}
		}
	}
}

bool Router::sendFlitAfterRequestCheck(uint8_t& request,
		std::queue<uint32_t>& flitFIFO, std::string creditString) {

	bool sentFlit = true;

	if (!flitFIFO.empty()) {
		uint32_t flit = flitFIFO.front();
		uint32_t flitType = get_flit_type(flit);

		if (flitType == HEADER_FLIT) {
			generateRequest(flit, request);
		}

		if (request != IDLE) {
			std::string reqString;

			// NORTH
			if ((request == NORTH_REQ) && (mCredit_Cnt_S > 0)) {
				reqString = "South";
				mCredit_Cnt_S--;
			}
			// EAST
			else if ((request == EAST_REQ) && (mCredit_Cnt_W > 0)) {
				reqString = "West";
				mCredit_Cnt_W--;
			}
			// WEST
			else if ((request == WEST_REQ) && (mCredit_Cnt_E > 0)) {
				reqString = "East";
				mCredit_Cnt_E--;
			}
			// SOUTH
			else if ((request == SOUTH_REQ) && (mCredit_Cnt_N > 0)) {
				reqString = "North";
				mCredit_Cnt_N--;
			}
			// LOCAL
			else if ((request == LOCAL_REQ)) {
				reqString = "Local_Packet_Sink";
			} else {
				sentFlit = false;
			}

			if (sentFlit) {
				this->sendFlit(flit, reqString);

				// Tail Flit
				if (flitType == TAIL_FLIT) {
//					std::cout << "**Complete** Packet was completed send by "
//							<< mName << std::endl;
//
//					std::cout << "<< RESET grants >>" << std::endl;
					if (request == NORTH_REQ) {
						north_grant = false;
					} else if (request == EAST_REQ) {
						east_grant = false;
					} else if (request == WEST_REQ) {
						west_grant = false;
					} else if (request == SOUTH_REQ) {
						south_grant = false;
					} else if (request == LOCAL_REQ) {
						local_grant = false;
					}

					request = IDLE;
				}

				flitFIFO.pop();
				this->updateCreditCounter(creditString);
			}
		} else {
			sentFlit = false;
		}
	}
// IDLE
	else {
		// No request (Router is IDLE)
		sentFlit = false;
	}

	return sentFlit;
}

void Router::generateRequest(uint32_t flit, uint8_t& request) {
//	std::cout << "[FLIT TYPE] --> " << flitType << std::endl;
//		std::cout << "GENERATE REQUEST: " << flit << std::endl;

	uint8_t parity = 0; // TODO not actually checked, but needed for receiving the flits
	uint16_t srcAddr = 0;
	uint16_t dstAddr = 0;
	bool n1 = false, e1 = false, w1 = false, s1 = false;
	parse_header_flit(flit, &dstAddr, &srcAddr, &parity);

//		std::cout << "*** " << mName << " Recv_" << mCurrentAddr << " - "
//				<< ", Src: " << srcAddr << ", Dst: " << dstAddr << std::dec
//				<< ", time: " << mCurrentSimTime << std::endl;

//		uint16_t des_addr_y = (dstAddr & PROPERTY_1_MASK) >> NOC_SIZE;
//		uint16_t cur_addr_y = (mCurrentAddr & PROPERTY_1_MASK) >> NOC_SIZE;
//		uint16_t des_addr_x = (dstAddr & PROPERTY_2_MASK);
//		uint16_t cur_addr_x = (mCurrentAddr & PROPERTY_2_MASK);

	uint16_t des_addr_x = dstAddr % NOC_SIZE;
	uint16_t cur_addr_x = mCurrentAddr % NOC_SIZE;
	uint16_t des_addr_y = dstAddr / NOC_SIZE;
	uint16_t cur_addr_y = mCurrentAddr / NOC_SIZE;

//		uint16_t des_addr_y = dstAddr % NOC_SIZE;
//		uint16_t cur_addr_y = mCurrentAddr % NOC_SIZE;
//		uint16_t des_addr_x = dstAddr / NOC_SIZE;
//		uint16_t cur_addr_x = mCurrentAddr / NOC_SIZE;

//		std::cout << mName << ": tmp_des_addr_x: " << des_addr_x << std::endl;
//		std::cout << mName << ": tmp_cur_addr_x: " << cur_addr_x << std::endl;
//
//		std::cout << mName << ": tmp_des_addr_y: " << des_addr_y << std::endl;
//		std::cout << mName << ": tmp_cur_addr_y: " << cur_addr_y << std::endl;

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

//		std::cout << mName << ": n1: " << std::boolalpha << n1 << std::endl;
//		std::cout << mName << ": e1: " << std::boolalpha << e1 << std::endl;
//
//		std::cout << mName << ": w1: " << std::boolalpha << w1 << std::endl;
//		std::cout << mName << ": s1: " << std::boolalpha << s1 << std::endl;

	// Generate Request
	if (((n1 && !e1 && !w1) || (n1 && e1 && mRoutingBits[0])
			|| (n1 && w1 && mRoutingBits[1])) && mConnectivityBits[0]
			&& (!north_grant)) {
		// Req_N
		request = NORTH_REQ;
		north_grant = true;
	} else if (((e1 && !n1 && !s1) || (e1 && n1 && mRoutingBits[2])
			|| (e1 && s1 && mRoutingBits[3])) && mConnectivityBits[1]
			&& (!east_grant)) {
		// Req_E
		request = EAST_REQ;
		east_grant = true;
	} else if (((w1 && !n1 && !s1) || (w1 && n1 && mRoutingBits[4])
			|| (w1 && s1 && mRoutingBits[5])) && mConnectivityBits[2]
			&& (!west_grant)) {
		// Req_W
		request = WEST_REQ;
		west_grant = true;
	} else if (((s1 && !e1 && !w1) || (s1 && e1 && mRoutingBits[6])
			|| (s1 && w1 && mRoutingBits[7])) && mConnectivityBits[3]
			&& (!south_grant)) {
		// Req_S
		request = SOUTH_REQ;
		south_grant = true;
	} else if ((!n1 && !e1 && !w1 && !s1) && (!local_grant)) {
		// Req_L
		request = LOCAL_REQ;
		local_grant = true;
	} else {
		request = IDLE;
	}
}

void Router::sendFlit(uint32_t flit, std::string reqString) {
	std::cout << mName << " sends " << flit << " to " << reqString
			<< " output of the next router" << std::endl;

	// Event Serialiazation
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	eventOffset = event::CreateEvent(fbb, fbb.CreateString(reqString),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(fbb, flit).Union());

	fbb.Finish(eventOffset);

	this->mPublisher.publishEvent(reqString, fbb.GetBufferPointer(),
			fbb.GetSize());
}

void Router::updateCreditCounter(std::string eventName) {
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	eventOffset = event::CreateEvent(fbb, fbb.CreateString(eventName),
			mCurrentSimTime);
	fbb.Finish(eventOffset);

	this->mPublisher.publishEvent(eventName, fbb.GetBufferPointer(),
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

	this->init();
}
