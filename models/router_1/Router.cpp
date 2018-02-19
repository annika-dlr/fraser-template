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

	// TODO: Connect to other routers ...

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
			mFlits_RX_L.push_back(mFlitData);
		} else {
			mNextFlit = 0; // TODO: Get Next Flit & Remove Flit from vector list
			this->generateRequests(mLocalRequests);
			this->sendFlit(this->getRequestWithHighestPriority(mNorthRequests));
		}
	}

	else if (mEventName == "North") {
		if (mFlits_RX_N.size() < 3) {
			mFlits_RX_N.push_back(mFlitData);
		} else {
			mNextFlit = 0; // TODO: Get Next Flit & Remove Flit from vector list
			this->generateRequests(mNorthRequests);
			this->sendFlit(this->getRequestWithHighestPriority(mNorthRequests));
		}

	}

	else if (mEventName == "East") {
		if (mFlits_RX_E.size() < 3) {
			mFlits_RX_E.push_back(mFlitData);
		} else {
			mNextFlit = 0; // TODO: Get Next Flit & Remove Flit from vector list
			this->generateRequests(mNorthRequests);
			this->sendFlit(this->getRequestWithHighestPriority(mNorthRequests));
		}
	}

	else if (mEventName == "South") {
		if (mFlits_RX_S.size() < 3) {
			mFlits_RX_S.push_back(mFlitData);
		} else {
			mNextFlit = 0; // TODO: Get Next Flit & Remove Flit from vector list
			this->generateRequests(mNorthRequests);
			this->sendFlit(this->getRequestWithHighestPriority(mNorthRequests));
		}
	}

	else if (mEventName == "West") {
		if (mFlits_RX_W.size() < 3) {
			mFlits_RX_W.push_back(mFlitData);
		} else {
			mNextFlit = 0; // TODO: Get Next Flit & Remove Flit from vector list
			this->generateRequests(mNorthRequests);
			this->sendFlit(this->getRequestWithHighestPriority(mNorthRequests));
		}
	}

	else if (mEventName == "End") {
		mRun = false;
	}
}

void Router::generateRequests(std::list<std::string> reqs) {
	mFlitType = get_flit_type(mNextFlit);
	if (mFlitType == FLIT_TYPE_HEADER) {
		int srcAddr = (mNextFlit & FLIT_SCR_ADDR_MASK) >> (SRC_ADDR_OFFSET + 1);
		int dstAddr = (mNextFlit & FLIT_DST_ADDR_MASK) >> (DST_ADDR_OFFSET + 1);

		// TODO: Generate Requests
		// Add request to reqs-list
	} else if (mFlitType == FLIT_TYPE_TAIL) {
		reqs.clear();
	}
}

std::string Router::getRequestWithHighestPriority(std::list<std::string> reqs) {
	std::string prioritizedReq = "";

	// TODO: Get highest priority request
	// North, East, West, South, Local, ...

	return prioritizedReq;
}

void Router::sendFlit(std::string req) {
	mEventOffset = event::CreateEvent(mFbb, mFbb.CreateString(req),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(mFbb, mNextFlit).Union());

	mFbb.Finish(mEventOffset);
	this->mPublisher.publishEvent(req, mFbb.GetBufferPointer(), mFbb.GetSize());
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
