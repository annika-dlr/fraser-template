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

#ifndef FRASER_TEMPLATE_MODELS_SYSTEMC_ADAPTER_SYSTEMCADAPTER_H_
#define FRASER_TEMPLATE_MODELS_SYSTEMC_ADAPTER_SYSTEMCADAPTER_H_

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include <string>
#include <iostream>
#include <zmq.hpp>

#include "interfaces/IModel.h"
#include "communication/zhelpers.hpp"
#include "communication/Subscriber.h"
#include "communication/Publisher.h"
#include "communication/Dealer.h"
#include "resources/idl/event_generated.h"

/** Receives Data from SystemC-models and forward it to FRASER specific models (publish data). **/
class SystemcAdapter: public virtual IModel, public sc_core::sc_module {

public:
	SystemcAdapter(std::string name, std::string description,
			sc_core::sc_module_name instance_name);
	virtual ~SystemcAdapter();

	// IModel
	virtual void init() override;
	virtual bool prepare() override;
	virtual void run() override;
	virtual std::string getName() const override {
		return mName;
	}
	virtual std::string getDescription() const override {
		return mDescription;
	}

	// TLM
	void b_transport(tlm::tlm_generic_payload &gp, sc_core::sc_time &time);

	// TLM-2 socket, defaults to 32-bits wide, base protocol
	tlm_utils::simple_target_socket<SystemcAdapter> mTargetSocket {
			"target_socket" };
	tlm_utils::simple_initiator_socket<SystemcAdapter> mInitMemorySocket {
			"init_socket_0" };
	tlm_utils::simple_initiator_socket<SystemcAdapter> mInitCreditCntSocket {
			"init_socket_1" };
	tlm_utils::simple_initiator_socket<SystemcAdapter> mInitInterruptSocket {
				"init_socket_2" };

private:
	// IModel
	std::string mName;
	std::string mDescription;

	// For the communication
	zmq::context_t mCtx;  // ZMQ-instance
	Subscriber mSubscriber; // ZMQ-SUB
	Publisher mPublisher; // ZMQ-PUB
	Dealer mDealer;		  // ZMQ-DEALER

	// Subscriber
	void handleEvent();

	bool mRun = false;
	uint32_t mCurrentSimTime = 0;

};

#endif /* FRASER_TEMPLATE_MODELS_SYSTEMC_ADAPTER_SYSTEMCADAPTER_H_ */
