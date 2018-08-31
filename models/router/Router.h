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

#ifndef FRASER_TEMPLATE_MODELS_ROUTER_1_ROUTER_H_
#define FRASER_TEMPLATE_MODELS_ROUTER_1_ROUTER_H_

#include <fstream>
#include <queue>
#include <list>
#include <string>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <zmq.hpp>
#include <stdint.h>
#include <bitset>

#include "resources/idl/event_generated.h"
#include "communication/zhelpers.hpp"
#include "communication/Subscriber.h"
#include "communication/Publisher.h"
#include "communication/Dealer.h"
#include "interfaces/IModel.h"
#include "interfaces/IPersist.h"
#include "data-types/Field.h"

#define NOC_SIZE 2

class Router: public virtual IModel, public virtual IPersist {
public:

	Router(std::string name, std::string description);
	virtual ~Router();

	enum Request : uint8_t {
		idle = 0,
		local = 1,
		north = 2,
		east = 3,
		south = 4,
		west = 5
	};

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

	// IPersist
	virtual void saveState(std::string filename) override;
	virtual void loadState(std::string filename) override;

private:
	// IModel
	std::string mName;
	std::string mDescription;

	// Subscriber
	void handleEvent();

	zmq::context_t mCtx;
	Subscriber mSubscriber;
	Publisher mPublisher;
	Dealer mDealer;

	bool mRun;
	uint32_t mCurrentSimTime = 0;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& archive, const unsigned int) {
	}

	uint64_t mCycles = 0;
	uint16_t mCurrentAddr = 0;
	std::bitset<16> mConnectivityBits = 0;
	std::bitset<16> mRoutingBits = 0;

	// Credit based flow
	uint16_t mCredit_Cnt_W = 3;
	uint16_t mCredit_Cnt_N = 3;
	uint16_t mCredit_Cnt_S = 3;
	uint16_t mCredit_Cnt_E = 3;

	std::queue<uint32_t> mFlits_RX_L; // L FIFO
	std::queue<uint32_t> mFlits_RX_N; // N FIFO
	std::queue<uint32_t> mFlits_RX_E; // E FIFO
	std::queue<uint32_t> mFlits_RX_S; // S FIFO
	std::queue<uint32_t> mFlits_RX_W; // W FIFO

	// Generated requests from LDBRs
	Request mReq_N_LBDR = idle;
	Request mReq_E_LBDR = idle;
	Request mReq_W_LBDR = idle;
	Request mReq_S_LBDR = idle;
	Request mReq_L_LBDR = idle;

	// Generated requests from LDBRs
	bool north_grant = false;
	bool east_grant = false;
	bool west_grant = false;
	bool south_grant = false;
	bool local_grant = false;

	bool sendFlitAfterRequestCheck(Request& request, std::queue<uint32_t>& flitFIFO, std::string creditString);
	void sendFlitWithRoundRobinPrioritization();
	// returns the request (Local, North, East, South or West)
	void generateRequest(uint32_t flit, Request& request);
	void sendFlit(uint32_t, std::string reqString);
	void updateCreditCounter(std::string eventName);
};

#endif /* FRASER_TEMPLATE_MODELS_ROUTER_1_ROUTER_H_ */
