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

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("event_queue_1"),
			mDealer.getPortNumFrom("event_queue_1"))) {
		return false;
	}

	for (auto depModel : mDealer.getAllModelDependencies(mName)) {
		if (!mSubscriber.connectToPub(mDealer.getIPFrom(depModel),
				mDealer.getPortNumFrom(depModel))) {
			return false;
		}
	}

	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");

	mSubscriber.subscribeTo("North");
	mSubscriber.subscribeTo("West");
	mSubscriber.subscribeTo("South");
	mSubscriber.subscribeTo("East");
	mSubscriber.subscribeTo("Local");

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

	if (mEventName == "SaveState") {

		this->saveState(
				std::string(mConfigPath.begin(), mConfigPath.end()) + mName
						+ ".config");
	}

	else if (mEventName == "LoadState") {
		this->loadState(
				std::string(mConfigPath.begin(), mConfigPath.end()) + mName
						+ ".config");
	}

	else if (mEventName == "Local") {
		mState = 0b10000;

		if (mFlits_RX_L.size() < 3) {
			mFlits_RX_L.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_L.front();
			mFlits_RX_L.pop();
			this->generateRequests(mReq_L_LBDR, mFlits_RX_L.empty());
			this->sendFlit(this->getRequestWithHighestPriority(mReq_L_LBDR));
		}
	}

	else if (mEventName == "North") {
		mState = 0b00001;

		if (mFlits_RX_N.size() < 3) {
			mFlits_RX_N.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_N.front();
			mFlits_RX_N.pop();
			this->generateRequests(mReq_N_LBDR, mFlits_RX_N.empty());
			this->sendFlit(this->getRequestWithHighestPriority(mReq_N_LBDR));
		}

	}

	else if (mEventName == "East") {
		mState = 0b00010;

		if (mFlits_RX_E.size() < 3) {
			mFlits_RX_E.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_E.front();
			mFlits_RX_E.pop();
			this->generateRequests(mReq_E_LBDR, mFlits_RX_E.empty());
			this->sendFlit(this->getRequestWithHighestPriority(mReq_E_LBDR));
		}
	}

	else if (mEventName == "South") {
		mState = 0b01000;

		if (mFlits_RX_S.size() < 3) {
			mFlits_RX_S.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_S.front();
			mFlits_RX_S.pop();
			this->generateRequests(mReq_S_LBDR, mFlits_RX_S.empty());
			this->sendFlit(this->getRequestWithHighestPriority(mReq_S_LBDR));
		}
	}

	else if (mEventName == "West") {
		mState = 0b00100;

		if (mFlits_RX_W.size() < 3) {
			mFlits_RX_W.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_W.front();
			mFlits_RX_W.pop();
			this->generateRequests(mReq_W_LBDR, mFlits_RX_W.empty());
			this->sendFlit(this->getRequestWithHighestPriority(mReq_W_LBDR));
		}
	}

	else if (mEventName == "End") {
		mRun = false;
	}
}

void Router::generateRequests(std::list<uint8_t> reqs, bool emptyFIFO) {
	mFlitType = get_flit_type(mNextFlit);
	if (mFlitType == FLIT_TYPE_HEADER && (!emptyFIFO)) {
		int srcAddr = 0;
		int dstAddr = 0;
		bool n1, e1, w1, s1 = 0;
		header_decode(mNextFlit, &srcAddr, &dstAddr, NOC_SIZE);

		int tmp_des_addr_ns = (dstAddr & MASK_PROPERTY_NS) >> 2;
		int tmp_cur_addr_ns = (srcAddr & MASK_PROPERTY_NS) >> 2;
		int tmp_des_addr_ew = (dstAddr & MASK_PROPERTY_EW);
		int tmp_cur_addr_ew = (srcAddr & MASK_PROPERTY_EW);

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
			reqs.push_back(0b00001);
		} else if ((e1 && !n1 && !s1) || (e1 && n1) || (e1 && s1)) {
			// Req_E
			reqs.push_back(0b00010);
		} else if ((w1 && !n1 && !s1) || (w1 && n1) || (w1 && s1)) {
			// Req_W
			reqs.push_back(0b00100);
		} else if (s1 && !e1 && !w1) {
			// Req_S
			reqs.push_back(0b01000);
		} else /*if (!n1 && !e1 && !w1 && !s1)*/{
			// Req_L
			reqs.push_back(0b10000);
		}
	}

	else if (mFlitType == FLIT_TYPE_TAIL) {
		reqs.clear();
	}

}

uint8_t Router::getRequestWithHighestPriority(std::list<uint8_t> reqs) {
	uint8_t prioritizedReq = 0;

	if (reqs.size() > 1) {
		for (auto req : reqs) {
			if (mState == req) {
				prioritizedReq = req;
			} else if ((mState << 1) == req) {
				prioritizedReq = req;
			}
		}
	} else {
		prioritizedReq = reqs.front();
	}

	return prioritizedReq;
}

void Router::sendFlit(uint8_t req) {
//	mEventOffset = event::CreateEvent(mFbb, mFbb.CreateString(req),
//			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
//			event::EventData_Flit, event::CreateFlit(mFbb, mNextFlit).Union());
//
//	mFbb.Finish(mEventOffset);

//this->mPublisher.publishEvent(req, mFbb.GetBufferPointer(), mFbb.GetSize());
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
