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

#ifndef FRASER_TEMPLATE_MODELS_PROCESSINGELEMENT_H_
#define FRASER_TEMPLATE_MODELS_PROCESSINGELEMENT_H_

#include <fstream>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <zmq.hpp>
#include <string>
#include <stdint.h>

#include "communication/zhelpers.hpp"
#include "communication/Subscriber.h"
#include "communication/Publisher.h"
#include "communication/Dealer.h"
#include "interfaces/IModel.h"
#include "interfaces/IPersist.h"
#include "data-types/Field.h"

#include "resources/idl/event_generated.h"
#include "packet_generator.h"
#include "packet_sink.h"

class ProcessingElement: public virtual IModel, public virtual IPersist {
public:
	ProcessingElement(std::string name, std::string description);
	virtual ~ProcessingElement();

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
	void saveState(std::string filename);
	void loadState(std::string filename);

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

	uint16_t mAddress = 0;
	uint16_t mCredit_Cnt_L = 3;

	bool mRun;
	int mCurrentSimTime;
	PacketGenerator mPacketGenerator;
	PacketSink mPacketSink;
	std::queue<uint32_t> mPacket;

	// Fields
	Field<uint16_t> mPacketNumber;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& archive, const unsigned int) {
		archive & boost::serialization::make_nvp("IntField", mPacketNumber);
	}
};

#endif /* FRASER_TEMPLATE_RESOURCES_SRC_PROCESSINGELEMENT_H_ */
