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

#include "RouterAdapter.h"

RouterAdapter::RouterAdapter(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mNocSize("NocSize", 2), mAddress(
				"RouterAddress", "0000"), mConnectivityBits("ConnectivityBits",
				"0000"), mRoutingBits("RoutingBits", "00000000") {

	registerInterruptSignal();

	mRun = this->prepare();
	//this->init();
}

RouterAdapter::~RouterAdapter() {

}

void RouterAdapter::init() {
	// Set or calculate other parameters ...

	mRouter.setNocSize(mNocSize.getValue());
	mRouter.setAddress(
			static_cast<uint16_t>(std::bitset<16>(mAddress.getValue()).to_ulong()));

	auto connectivityBits = std::bitset<16>(mConnectivityBits.getValue());
	mRouter.setConnectivityBits(connectivityBits);
	mRouter.setRoutingBits(std::bitset<16>(mRoutingBits.getValue()));

	if (connectivityBits[0]) {
		mSubscriber.subscribeTo("South");
		mSubscriber.subscribeTo("Credit_in_S++");

	}
	if (connectivityBits[1]) {
		mSubscriber.subscribeTo("West");
		mSubscriber.subscribeTo("Credit_in_W++");

	}
	if (connectivityBits[2]) {
		mSubscriber.subscribeTo("East");
		mSubscriber.subscribeTo("Credit_in_E++");
	}
	if (connectivityBits[3]) {
		mSubscriber.subscribeTo("North");
		mSubscriber.subscribeTo("Credit_in_N++");
	}
}

bool RouterAdapter::prepare() {
	mSubscriber.setOwnershipName(mName);

	// Router initialization for the default configuration file
	// Otherwise, all parameters would be initialized with zero
	// Parameters are defined in the hosts-configuration file (hosts-configs/)
	// TODO: Just use one of the both options to save/store parameters
	mAddress.setValue(mDealer.getModelParameter(mName, "address"));
	mRoutingBits.setValue(mDealer.getModelParameter(mName, "routingBits"));
	mConnectivityBits.setValue(
			mDealer.getModelParameter(mName, "connectivityBits"));

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

	// Subscriptions to events
	mSubscriber.subscribeTo("LoadState");
	mSubscriber.subscribeTo("SaveState");
	mSubscriber.subscribeTo("End");
	mSubscriber.subscribeTo("PacketGenerator");
	mSubscriber.subscribeTo("SimTimeChanged");

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

void RouterAdapter::run() {

	while (mRun) {
		if (mSubscriber.receiveEvent()) {
			this->handleEvent();
		}
	}
}

void RouterAdapter::handleEvent() {
	auto eventBuffer = mSubscriber.getEventBuffer();

	auto receivedEvent = event::GetEvent(eventBuffer);
	std::string eventName = receivedEvent->name()->str();
	mCurrentSimTime = receivedEvent->timestamp();
	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (receivedEvent->data_type() == event::EventData_Flit) {
		auto flitData = receivedEvent->data_as_Flit()->uint32();

		if (eventName == "PacketGenerator") {
			mRouter.pushToLocalFIFO(flitData);
		}
		// Flit comes from the South output of the previous router
		else if (eventName == "South") {
			mRouter.pushToNorthFIFO(flitData);
		}
		// Flit comes from the West output of the previous router
		else if (eventName == "West") {
			mRouter.pushToEastFIFO(flitData);

		}
		// Flit comes from the North output of the previous router
		else if (eventName == "North") {
			mRouter.pushToSouthFIFO(flitData);

		}
		// Flit comes from the East output of the previous router
		else if (eventName == "East") {
			mRouter.pushToWestFIFO(flitData);

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

		if (mRouter.arbitrateWithRoundRobinPrioritization()) {
			sendFlit(mRouter.getNextFlit(), mRouter.getChosenOutputPort());
			updateCreditCounter(mRouter.getCreditCntSignal());
		}
	}

	// Increase Credit Counter
	else if (eventName == "Credit_in_N++") {
		mRouter.increaseCreditCntNorth();
	}

	else if (eventName == "Credit_in_W++") {
		mRouter.increaseCreditCntWest();
	}

	else if (eventName == "Credit_in_E++") {
		mRouter.increaseCreditCntEast();
	}

	else if (eventName == "Credit_in_S++") {
		mRouter.increaseCreditCntSouth();
	}

	else if (eventName == "End") {
		mRun = false;
	}

}

void RouterAdapter::sendFlit(uint32_t flit, std::string reqString) {
	std::cout << mName << " sends " << flit << " to " << reqString << " output"
			<< std::endl;

	// Event Serialiazation
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	eventOffset = event::CreateEvent(fbb, fbb.CreateString(reqString),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(fbb, flit).Union());

	fbb.Finish(eventOffset);

	mPublisher.publishEvent(reqString, fbb.GetBufferPointer(), fbb.GetSize());
}

void RouterAdapter::updateCreditCounter(std::string eventName) {
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	eventOffset = event::CreateEvent(fbb, fbb.CreateString(eventName),
			mCurrentSimTime);
	fbb.Finish(eventOffset);

	mPublisher.publishEvent(eventName, fbb.GetBufferPointer(), fbb.GetSize());
}

void RouterAdapter::saveState(std::string filePath) {
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

void RouterAdapter::loadState(std::string filePath) {
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

	// Optional calculate parameters from the loaded initial state
	init();

	mRun = mSubscriber.synchronizeSub();
}
