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
 * - 2017-2018, Annika Ofenloch (DLR RY-AVS)
 */

#include <iostream>
#include "Model_1.h"

Model1::Model1(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mReceivedEvent(NULL), mCurrentSimTime(
				0) {

	mRun = this->prepare();
	this->init();
}

Model1::~Model1() {
}

void Model1::init() {
	// Set or calculate other parameters ...
}

bool Model1::prepare() {
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

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("model_2"),
			mDealer.getPortNumFrom("model_2"))) {
		return false;
	}


	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");
	mSubscriber.subscribeTo("PCDUCommand");
	mSubscriber.subscribeTo("FirstEvent");
	mSubscriber.subscribeTo("ReturnEvent");

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

void Model1::run() {
	while (mRun) {
		mSubscriber.receiveEvent();
		this->handleEvent();
	}
}

void Model1::handleEvent() {
	mReceivedEvent = event::GetEvent(mSubscriber.getEventBuffer());
	mEventName = mReceivedEvent->name()->str();
	mCurrentSimTime = mReceivedEvent->timestamp();
	mData = mReceivedEvent->data_as_String()->str();

	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (mEventName == "FirstEvent") {
		mEventOffset = event::CreateEvent(mFbb,
				mFbb.CreateString("SubsequentEvent"), mCurrentSimTime);
		mFbb.Finish(mEventOffset);
		this->mPublisher.publishEvent("SubsequentEvent",
				mFbb.GetBufferPointer(), mFbb.GetSize());
	}

	else if (mEventName == "ReturnEvent") {

	}

	else if (mEventName == "SaveState") {
		this->saveState(std::string(mData.begin(), mData.end()) + mName + ".config");
	}

	else if (mEventName == "LoadState") {
		this->loadState(std::string(mData.begin(), mData.end()) + mName + ".config");
	}

	else if (mEventName == "End") {
		mRun = false;
	}

}

void Model1::saveState(std::string filePath) {
	// Store states
	std::ofstream ofs(filePath);
	boost::archive::xml_oarchive oa(ofs, boost::archive::no_header);
	try {
		oa << boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ": Archive Exception during serializing:" << std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();
}

void Model1::loadState(std::string filePath) {
	// Restore states
	std::ifstream ifs(filePath);
	boost::archive::xml_iarchive ia(ifs, boost::archive::no_header);
	try {
		ia >> boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ":Archive Exception during deserializing:" << std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();

	this->init();
}
