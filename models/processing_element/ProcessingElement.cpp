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
#include <bitset>

ProcessingElement::ProcessingElement(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mCurrentSimTime(0), mPacketGenerator(
				0), mPacketSink(0), mPacketNumber("PacketNumber", 10) {

	mRun = this->prepare();
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

		// Set router address
		uint16_t address = static_cast<uint16_t>(std::bitset<16>(
				mDealer.getModelParameter(depModel, "address")).to_ulong());

		mPacketGenerator.set_local_address(address);
		mPacketSink.set_local_address(address);

		uint16_t destinationAddress = 1;
		if (destinationAddress == address) {
			destinationAddress = 2;
		}

		mPacketGenerator.generate_packet(mPacket, mPacketNumber.getValue(),
				destinationAddress, GenerationModes::counter, mCurrentSimTime);
		std::cout << "[PACKET SIZE] --> " << mPacket.size() << std::endl;
	}

	mSubscriber.subscribeTo("Local_Packet_Sink");
	mSubscriber.subscribeTo("Credit_in_L++");

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
		/*if (mPacket.empty()) {
		 mPacketGenerator.generate_packet(mPacket, mPacketNumber.getValue(),
		 1, GenerationModes::counter, mCurrentSimTime);
		 } else*/

		if (mSubscriber.receiveEvent()) {
			this->handleEvent();
		}
	}
}

void ProcessingElement::handleEvent() {
	auto eventBuffer = mSubscriber.getEventBuffer();

	// Event Serialization
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	auto receivedEvent = event::GetEvent(eventBuffer);

	std::string eventName = receivedEvent->name()->str();
	mCurrentSimTime = receivedEvent->timestamp();
	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (eventName == "SimTimeChanged") {

//		std::cout << mName << ": PACKET SIZE: " << mPacket.size() << std::endl;
		if (!mPacket.empty() && (mCredit_Cnt_L > 0)) {
			auto flit = mPacket.front();

			// Example for Sending (Publishing) a flit and using Flatbuffers to serialize the data (flit):
			std::string reqString = "Local";
			eventOffset = event::CreateEvent(fbb, fbb.CreateString(reqString),
					mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
					event::EventData_Flit,
					event::CreateFlit(fbb, flit).Union());
			fbb.Finish(eventOffset);

			std::cout << mName << " sends " << flit << " to " << reqString
					<< std::endl;

			mPublisher.publishEvent(reqString, fbb.GetBufferPointer(),
					fbb.GetSize());

			mPacket.pop();
			mCredit_Cnt_L--;
		}
	}

	else if (eventName == "Credit_in_L++") {
		if (mCredit_Cnt_L < 3) {
			mCredit_Cnt_L++;
		}
		std::cout << mName << " Credit_in_L++: " << mCredit_Cnt_L << std::endl;
	}

	else if (receivedEvent->data_type() == event::EventData_Flit) {
		auto flitData = receivedEvent->data_as_Flit()->uint32();

		if (eventName == "Local_Packet_Sink") {
			mPacketSink.send_flit_to_local(flitData, mCurrentSimTime);
		}
	}

	else if (receivedEvent->data_type() == event::EventData_String) {
		std::string configPath = receivedEvent->data_as_String()->str();

		if (eventName == "SaveState") {
			saveState(
					std::string(configPath.begin(), configPath.end()) + mName
							+ ".config");
		}

		else if (eventName == "LoadState") {
			loadState(
					std::string(configPath.begin(), configPath.end()) + mName
							+ ".config");
		}
	} else if (eventName == "End") {
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