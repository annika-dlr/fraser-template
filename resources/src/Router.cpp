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
		if (mFlits_RX_L.size() < 3) {
			mFlits_RX_L.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_L.front();
			mFlits_RX_L.pop();
			this->generateRequests(mL_LDBR_Reqs);
			this->sendFlit(this->getRequestWithHighestPriority(mL_LDBR_Reqs));
		}
	}

	else if (mEventName == "North") {
		if (mFlits_RX_N.size() < 3) {
			mFlits_RX_N.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_N.front();
			mFlits_RX_N.pop();
			this->generateRequests(mN_LDBR_Reqs);
			this->sendFlit(this->getRequestWithHighestPriority(mN_LDBR_Reqs));
		}

	}

	else if (mEventName == "East") {
		if (mFlits_RX_E.size() < 3) {
			mFlits_RX_E.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_E.front();
			mFlits_RX_E.pop();
			this->generateRequests(mE_LDBR_Reqs);
			this->sendFlit(this->getRequestWithHighestPriority(mE_LDBR_Reqs));
		}
	}

	else if (mEventName == "South") {
		if (mFlits_RX_S.size() < 3) {
			mFlits_RX_S.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_S.front();
			mFlits_RX_S.pop();
			this->generateRequests(mS_LDBR_Reqs);
			this->sendFlit(this->getRequestWithHighestPriority(mS_LDBR_Reqs));
		}
	}

	else if (mEventName == "West") {
		if (mFlits_RX_W.size() < 3) {
			mFlits_RX_W.push(mFlitData);
		} else {
			mNextFlit = mFlits_RX_W.front();
			mFlits_RX_W.pop();
			this->generateRequests(mW_LDBR_Reqs);
			this->sendFlit(this->getRequestWithHighestPriority(mW_LDBR_Reqs));
		}
	}

	else if (mEventName == "End") {
		mRun = false;
	}
}

void Router::generateRequests(std::list<uint8_t> reqs) {
	mFlitType = get_flit_type(mNextFlit);
	if (mFlitType == FLIT_TYPE_HEADER) {
		int srcAddr = 0;
		int dstAddr = 0;

		header_decode(mNextFlit, &srcAddr, &dstAddr, NOC_SIZE);

		// TODO: Generate Requests
		// Add request to reqs-list

		reqs.push_back(0); // Add request
	}
	// TODO: else if (flit = header and empty = 1) then all request should keep the previous value

	else if (mFlitType == FLIT_TYPE_TAIL) {
		reqs.clear();
	}

	// else if (flit != Header and flit != Tail) then all request should keep the previous value
}

uint8_t Router::getRequestWithHighestPriority(std::list<uint8_t> reqs) {
	uint8_t prioritizedReq = 0;

	// TODO: Get highest priority request
	// North, East, West, South, Local, ...

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
