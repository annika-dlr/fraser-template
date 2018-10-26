/*
 * Copyright (c) 2018, German Aerospace Center (DLR)
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

#include "SystemcAdapter.h"

SystemcAdapter::SystemcAdapter(std::string name, std::string description,
		sc_core::sc_module_name instance_name) :
		sc_core::sc_module(instance_name), mName(name), mDescription(
				description), mCtx(1), mSubscriber(mCtx), mPublisher(mCtx), mDealer(
				mCtx, mName) {

	// *********************************************
	// Register callbacks for incoming interface method calls
	// *********************************************
	mTargetSocket.register_b_transport(this, &SystemcAdapter::b_transport);

	mRun = prepare();

	SC_HAS_PROCESS(SystemcAdapter);
	SC_THREAD(run);
}

SystemcAdapter::~SystemcAdapter() {
}

void SystemcAdapter::init() {
	// Set or calculate other parameters ...
}

bool SystemcAdapter::prepare() {

	mSubscriber.setOwnershipName(mName);

	if (!mPublisher.bindSocket(mDealer.getPortNumFrom(mName))) {
		std::cout << mName << " could not bind to port "
				<< mDealer.getPortNumFrom(mName) << std::endl;
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
	mSubscriber.subscribeTo("SimTimeChanged");
	mSubscriber.subscribeTo("Credit_in_L++");
	mSubscriber.subscribeTo("Local");
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

void SystemcAdapter::b_transport(tlm::tlm_generic_payload& trans,
		sc_time& time) {
	uint32_t in_flit = 0;
	// parse the transaction
	unsigned char* ptr = trans.get_data_ptr();
	unsigned int len = trans.get_data_length();
//	unsigned char* byt = trans.get_byte_enable_ptr();
//	unsigned int wid = trans.get_streaming_width();

	memcpy(&in_flit, ptr, len);
	cout << "SystemcAdapter publishes Flit: " << mCurrentSimTime << " <-- "
			<< in_flit << endl;

	// Event Serialiazation
	flatbuffers::FlatBufferBuilder fbb;
	flatbuffers::Offset<event::Event> eventOffset;

	std::string reqString = "PacketGenerator";
	eventOffset = event::CreateEvent(fbb, fbb.CreateString(reqString),
			mCurrentSimTime, event::Priority_NORMAL_PRIORITY, 0, 0,
			event::EventData_Flit, event::CreateFlit(fbb, in_flit).Union());

	fbb.Finish(eventOffset);

	mPublisher.publishEvent(reqString, fbb.GetBufferPointer(), fbb.GetSize());

	trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

void SystemcAdapter::run() {
	cout << endl << "In system-adapter thread_process" << endl;

	while (mRun) {
		if (mSubscriber.receiveEvent()) {
			this->handleEvent();
		}
	}
}

void SystemcAdapter::handleEvent() {
	auto eventBuffer = mSubscriber.getEventBuffer();
	auto receivedEvent = event::GetEvent(eventBuffer);

	std::string eventName = receivedEvent->name()->str();
	mCurrentSimTime = receivedEvent->timestamp();
	mRun = !foundCriticalSimCycle(mCurrentSimTime);
	sc_time delay = sc_time(100, SC_MS);

	if (receivedEvent->data_type() == event::EventData_Flit) {
		auto flit = receivedEvent->data_as_Flit()->uint32();

		// TLM-2 generic payload transaction, reused across calls to b_transport
		tlm::tlm_generic_payload* flitTrans = new tlm::tlm_generic_payload;

		cout << "Flit from pkt-gen: " << flit << endl;

		// set the transaction
		flitTrans->set_data_ptr(reinterpret_cast<unsigned char*>(&flit));
		flitTrans->set_data_length(4);
		flitTrans->set_streaming_width(4);
		flitTrans->set_byte_enable_ptr(0);
		flitTrans->set_dmi_allowed(false);
		flitTrans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

		mInitMemorySocket->b_transport(*flitTrans, delay); //blocking call
	}

	else if (eventName == "Credit_in_L++") {
		// TLM-2 generic payload transaction, reused across calls to b_transport
		tlm::tlm_generic_payload* creditCntTrans = new tlm::tlm_generic_payload;
		// set the transaction
		uint32_t data = 0;
		creditCntTrans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
		creditCntTrans->set_data_length(4);
		creditCntTrans->set_streaming_width(4);
		creditCntTrans->set_byte_enable_ptr(0);
		creditCntTrans->set_dmi_allowed(false);
		creditCntTrans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

		mInitCreditCntSocket->b_transport(*creditCntTrans, delay);
	}

	else if (eventName == "End") {
		mRun = false;

		tlm::tlm_generic_payload* interruptTrans = new tlm::tlm_generic_payload;
		// set the transaction
		uint32_t data = 0;
		interruptTrans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
		interruptTrans->set_data_length(4);
		interruptTrans->set_streaming_width(4);
		interruptTrans->set_byte_enable_ptr(0);
		interruptTrans->set_dmi_allowed(false);
		interruptTrans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

		mInitInterruptSocket->b_transport(*interruptTrans, delay);
	}

	wait(delay);
}

