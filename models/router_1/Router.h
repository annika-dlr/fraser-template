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
#include <boost/serialization/serialization.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <zmq.hpp>

#include "Packets.h"
#include "resources/idl/event_generated.h"
#include "communication/zhelpers.hpp"
#include "communication/Subscriber.h"
#include "communication/Publisher.h"
#include "communication/Dealer.h"
#include "interfaces/IModel.h"
#include "interfaces/IPersist.h"
#include "data-types/Field.h"
#include "resources/idl/event_generated.h"

#define NOC_SIZE 4

class Router: public virtual IModel, public virtual IPersist {
public:
	Router(std::string name, std::string description);
	virtual ~Router();

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
	const event::Event* mReceivedEvent;
	std::string mEventName;
	int mCurrentSimTime;
	std::string mConfigPath;
	uint32_t mFlitData;
	uint32_t mFlitType;

	// Event Serialiazation
	flatbuffers::FlatBufferBuilder mFbb;
	flatbuffers::Offset<event::Event> mEventOffset;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& archive, const unsigned int) {
	}

	uint64_t mCycles = 0;
	uint16_t mCredits = 3;
	bool mLastReveived = false;
	uint32_t mNextFlit = 0;

	std::vector<uint32_t> mFlits_RX_L; // FIFO
	std::vector<uint32_t> mFlits_RX_N; // FIFO
	std::vector<uint32_t> mFlits_RX_E; // FIFO
	std::vector<uint32_t> mFlits_RX_S; // FIFO
	std::vector<uint32_t> mFlits_RX_W; // FIFO

	std::list<std::string> mLocalRequests; // L LBDR
	std::list<std::string> mNorthRequests; // N LBDR
	std::list<std::string> mEastRequests;  // E LBDR
	std::list<std::string> mSouthRequests; // S LBDR
	std::list<std::string> mWestRequests;  // W LBDR

	void generateRequests(std::list<std::string> reqs);
	std::string getRequestWithHighestPriority(std::list<std::string> reqs);
	void sendFlit(std::string req);
};

#endif /* FRASER_TEMPLATE_MODELS_ROUTER_1_ROUTER_H_ */
