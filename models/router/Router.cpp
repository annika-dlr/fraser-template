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
#include <bitset>


Router::Router(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mReceivedEvent(NULL), mCurrentSimTime(
				0), mFlitData(0), mFlitType(0) {

	mRun = this->prepare();
	this->init();
}

Router::~Router() {

}

void Router::init() {
	// Set or calculate other parameters ...

	mCurrentAddr = static_cast<uint16_t>(std::bitset<16>(mDealer.getModelParameter(mName, "address")).to_ulong());
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

	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");

	mSubscriber.subscribeTo("North_Req");
	mSubscriber.subscribeTo("West_Req");
	mSubscriber.subscribeTo("South_Req");
	mSubscriber.subscribeTo("East_Req");
	// FIXME: Connected to the PE
	//mSubscriber.subscribeTo("Local_Req");

	mSubscriber.subscribeTo("Credit_in_N");
	mSubscriber.subscribeTo("Credit_in_W");
	mSubscriber.subscribeTo("Credit_in_S");
	mSubscriber.subscribeTo("Credit_in_E");
	// FIXME: Connected to the PE
	//mSubscriber.subscribeTo("Credit_in_L");

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
		mSubscriber.receiveEvent();
		this->handleEvent();
	}
}

void Router::handleEvent() {
	mReceivedEvent = event::GetEvent(mSubscriber.getEventBuffer());
	mEventName = mReceivedEvent->name()->str();
	mCurrentSimTime = mReceivedEvent->timestamp();

	if (mReceivedEvent->data_type() == event::EventData_Flit) {
		mFlitData = mReceivedEvent->data_as_Flit()->uint32();

	} else if (mReceivedEvent->data_type() == event::EventData_String) {
		mConfigPath = mReceivedEvent->data_as_String()->str();
	}

	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (mEventName == "Local_Req") {
		mCurrentState = LOCAL_REQ;

		if (mFlits_RX_L.size() < 3) {
			mFlits_RX_L.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_L.front();
			mFlits_RX_L.pop();
			this->generateRequests(mReq_L_LBDR, mFlits_RX_L.empty());

			if (mCredit_Cnt_L > 0 && !mReq_L_LBDR.empty()) {
				this->sendFlit(
						this->getRequestWithHighestPriority(mReq_L_LBDR));
				mCredit_Cnt_L--;
				this->updateCreditCounter("Credit_in_L");
			}
		}
	}

	else if (mEventName == "North_Req") {
		mCurrentState = NORTH_REQ;

		if (mFlits_RX_N.size() < 3) {
			mFlits_RX_N.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_N.front();
			mFlits_RX_N.pop();
			this->generateRequests(mReq_N_LBDR, mFlits_RX_N.empty());

			if (mCredit_Cnt_N > 0 && !mReq_N_LBDR.empty()) {
				this->sendFlit(
						this->getRequestWithHighestPriority(mReq_N_LBDR));
				mCredit_Cnt_N--;
				this->updateCreditCounter("Credit_in_N");
			}
		}
	}

	else if (mEventName == "East_Req") {
		mCurrentState = EAST_REQ;

		if (mFlits_RX_E.size() < 3) {
			mFlits_RX_E.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_E.front();
			mFlits_RX_E.pop();
			this->generateRequests(mReq_E_LBDR, mFlits_RX_E.empty());

			if (mCredit_Cnt_E > 0 && !mReq_E_LBDR.empty()) {
				this->sendFlit(
						this->getRequestWithHighestPriority(mReq_E_LBDR));
				mCredit_Cnt_E--;
				this->updateCreditCounter("Credit_in_E");
			}
		}
	}

	else if (mEventName == "South_Req") {
		mCurrentState = SOUTH_REQ;

		if (mFlits_RX_S.size() < 3) {
			mFlits_RX_S.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_S.front();
			mFlits_RX_S.pop();
			this->generateRequests(mReq_S_LBDR, mFlits_RX_S.empty());

			if (mCredit_Cnt_S > 0 && !mReq_S_LBDR.empty()) {
				this->sendFlit(
						this->getRequestWithHighestPriority(mReq_S_LBDR));
				mCredit_Cnt_S--;
				this->updateCreditCounter("Credit_in_S");
			}
		}
	}

	else if (mEventName == "West_Req") {
		mCurrentState = WEST_REQ;

		if (mFlits_RX_W.size() < 3) {
			mFlits_RX_W.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_W.front();
			mFlits_RX_W.pop();
			this->generateRequests(mReq_W_LBDR, mFlits_RX_W.empty());

			if (mCredit_Cnt_W > 0 && !mReq_W_LBDR.empty()) {
				this->sendFlit(
						this->getRequestWithHighestPriority(mReq_W_LBDR));
				this->updateCreditCounter("Credit_in_W");
			}
		}
	}

	else if (mEventName == "Credit_in_N") {
		mCredit_Cnt_N++;
	}

	else if (mEventName == "Credit_in_W") {
		mCredit_Cnt_W++;
	}

	else if (mEventName == "Credit_in_E") {
		mCredit_Cnt_E++;
	}

	else if (mEventName == "Credit_in_L") {
		mCredit_Cnt_L++;
	}

	else if (mEventName == "Credit_in_S") {
		mCredit_Cnt_S++;
	}

	else if (mEventName == "SaveState") {
		this->saveState(
				std::string(mConfigPath.begin(), mConfigPath.end()) + mName
						+ ".config");
	}

	else if (mEventName == "LoadState") {
		this->loadState(
				std::string(mConfigPath.begin(), mConfigPath.end()) + mName
						+ ".config");
	}

	else if (mEventName == "End") {
		mRun = false;
	}
}

void Router::generateRequests(std::list<uint8_t> reqs, bool emptyFIFO) {
	mFlitType = get_flit_type(mNextFlit);
	if (mFlitType == FLIT_TYPE_HEADER && (!emptyFIFO)) {
		std::cout<<"Flit type = HEADER"<<std::endl;
		int srcAddr = 0;
		int dstAddr = 0;
		bool n1, e1, w1, s1 = 0;
		header_decode(mNextFlit, &srcAddr, &dstAddr, NOC_SIZE);

		int tmp_des_addr_ns = (dstAddr & PROPERTY_1_MASK) >> 2;
		int tmp_cur_addr_ns = (mCurrentAddr & PROPERTY_1_MASK) >> 2;
		int tmp_des_addr_ew = (dstAddr & PROPERTY_2_MASK);
		int tmp_cur_addr_ew = (mCurrentAddr & PROPERTY_2_MASK);

		if (tmp_des_addr_ns < tmp_cur_addr_ns) {
			n1 = 1;
		}
		if (tmp_cur_addr_ew < tmp_des_addr_ew) {
			e1 = 1;
		}
		if (tmp_des_addr_ew < tmp_cur_addr_ew) {
			w1 = 1;
		}
		if (tmp_cur_addr_ns < tmp_des_addr_ns) {
			s1 = 1;
		}

		// Generate Request
		if (n1 && !e1 && !w1) {
			// Req_N
			reqs.push_back(NORTH_REQ);
		} else if ((e1 && !n1 && !s1) || (e1 && n1) || (e1 && s1)) {
			// Req_E
			reqs.push_back(EAST_REQ);
		} else if ((w1 && !n1 && !s1) || (w1 && n1) || (w1 && s1)) {
			// Req_W
			reqs.push_back(WEST_REQ);
		} else if (s1 && !e1 && !w1) {
			// Req_S
			reqs.push_back(SOUTH_REQ);
		} else /*if (!n1 && !e1 && !w1 && !s1)*/{
			// Req_L
			reqs.push_back(LOCAL_REQ);
		}
	}

	else if (mFlitType == FLIT_TYPE_TAIL) {
		std::cout<<"Flit type = TAIL"<<std::endl;
		reqs.clear();
	}

}

uint8_t Router::getRequestWithHighestPriority(std::list<uint8_t> reqs) {
	uint8_t prioritizedReq = 0;

	if (reqs.size() > 1) {
		for (auto req : reqs) {
			if (mCurrentState == req) {
				prioritizedReq = req;
			} else if ((mCurrentState << 1) == req) {
				prioritizedReq = req;
			}
		}
	} else {
		prioritizedReq = reqs.front();
	}

	return prioritizedReq;
}

void Router::sendFlit(uint8_t req) {
	std::string reqString = "";

	if (req == NORTH_REQ) {
		reqString = "North_Req";
	} else if (req == LOCAL_REQ) {
		reqString = "Local_Req";
	} else if (req == EAST_REQ) {
		reqString = "East_Req";
	} else if (req == WEST_REQ) {
		reqString = "West_Req";
	} else if (req == SOUTH_REQ) {
		reqString = "South_Req";
	}

	mEventOffset = event::CreateEvent(mFbb, mFbb.CreateString(reqString),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(mFbb, mNextFlit).Union());

	mFbb.Finish(mEventOffset);

	this->mPublisher.publishEvent(reqString, mFbb.GetBufferPointer(),
			mFbb.GetSize());
}

void Router::updateCreditCounter(std::string eventName) {
	mEventOffset = event::CreateEvent(mFbb, mFbb.CreateString(eventName),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(mFbb, mNextFlit).Union());

	mFbb.Finish(mEventOffset);

	this->mPublisher.publishEvent(eventName, mFbb.GetBufferPointer(),
			mFbb.GetSize());
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
