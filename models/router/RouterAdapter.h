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

#ifndef FRASER_TEMPLATE_MODELS_ROUTER_ROUTERADAPTER_H_
#define FRASER_TEMPLATE_MODELS_ROUTER_ROUTERADAPTER_H_

#include <fstream>
#include <queue>
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
#include "router/router.h"

class RouterAdapter: public virtual IModel, public virtual IPersist {
public:

	RouterAdapter(std::string name, std::string description);
	virtual ~RouterAdapter();

	// IModel
	virtual void init() override;
	virtual bool prepare() override;
	virtual void run() override;
	// IModel
	virtual std::string getName() const override {
		return mName;
	}
	// IModel
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

	bool mRun = false;
	uint32_t mCurrentSimTime = 0;

	Router mRouter;
	void sendFlit(uint32_t, std::string reqString);
	void updateCreditCounter(std::string signal);

	// Fields
	Field<uint16_t> mNocSize;
	Field<std::string> mAddress;
	Field<std::string> mConnectivityBits;
	Field<std::string> mRoutingBits;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& archive, const unsigned int) {
		archive & boost::serialization::make_nvp("IntField", mNocSize);
		archive & boost::serialization::make_nvp("BitSetField", mAddress);
		archive & boost::serialization::make_nvp("BitSetField", mConnectivityBits);
		archive & boost::serialization::make_nvp("BitSetField", mRoutingBits);
	}
};

#endif /* FRASER_TEMPLATE_MODELS_ROUTER_1_ROUTER_H_ */
