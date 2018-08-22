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

#include <vector>
#include <cstdint>
#include "ProcessingElement.h"
#include "packet_generator.h"

ProcessingElement::ProcessingElement(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mReceivedEvent(NULL), mCurrentSimTime(
				0) {

	mRun = this->prepare();
	this->init();
}

ProcessingElement::~ProcessingElement() {
	// delete this->mPacketGenerator;
}

void ProcessingElement::init() {
	// Set or calculate other parameters ...
}

bool ProcessingElement::prepare() {
	mSubscriber.setOwnershipName(mName);

	if (!mPublisher.bindSocket(mDealer.getPortNumFrom(mName))) {
		return false;
	}

	// Connect to simulation model (global clock) to receive the current simulation time
	if (!mSubscriber.connectToPub(mDealer.getIPFrom("simulation_model"),
			mDealer.getPortNumFrom("simulation_model"))) {
		return false;
	}

	// Connect to all models (in this case the corresponding router) it depends on
	for (auto depModel : mDealer.getModelDependencies()) {
		if (!mSubscriber.connectToPub(mDealer.getIPFrom(depModel),
				mDealer.getPortNumFrom(depModel))) {
			return false;
		}
	}

	mSubscriber.subscribeTo("SimTimeChanged");
	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");

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

void ProcessingElement::run() {
	while (mRun) {
		mSubscriber.receiveEvent();
		this->handleEvent();
	}
}

void ProcessingElement::handleEvent() {
	mReceivedEvent = event::GetEvent(mSubscriber.getEventBuffer());
	mEventName = mReceivedEvent->name()->str();
	mCurrentSimTime = mReceivedEvent->timestamp();
	mData = mReceivedEvent->data_as_String()->str();

	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (mEventName == "SimTimeChanged") {

		PacketGenerator mPacketGenerator(0); //FIXME: Get router's address, this object should be loaded on model initialization

		std::vector<uint32_t> packet;

		mPacketGenerator.generate_packet(packet, 10, 1, GenerationModes::counter);

		for (size_t i=0; i < packet.size(); i++) {
			std::cout << "Sending ", packet.at(i);
		}

		// Receives current simulation time ...
		// Do something for each time step

		// Example for Sending (Publishing) a flit and using Flatbuffers to serialize the data (flit):
//		uint32_t flit;
//		std::string reqString = "North_Req";
//		mEventOffset = event::CreateEvent(mFbb, mFbb.CreateString(reqString),
//				mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
//				event::EventData_Flit,
//				event::CreateFlit(mFbb, flit).Union());
//
//		mFbb.Finish(mEventOffset);
//
//		this->mPublisher.publishEvent(reqString, mFbb.GetBufferPointer(),
//				mFbb.GetSize());
	}

	else if (mEventName == "SaveState") {
		this->saveState(
				std::string(mData.begin(), mData.end()) + mName + ".config");
	}

	else if (mEventName == "LoadState") {
		this->loadState(
				std::string(mData.begin(), mData.end()) + mName + ".config");
	}

	else if (mEventName == "End") {
		mRun = false;
	}

}

void ProcessingElement::saveState(std::string filePath) {
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

void ProcessingElement::loadState(std::string filePath) {
	// Restore states
	std::ifstream ifs(filePath);
	boost::archive::xml_iarchive ia(ifs, boost::archive::no_header);
	try {
		ia >> boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ":Archive Exception during deserializing:"
				<< std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();

	this->init();
}
