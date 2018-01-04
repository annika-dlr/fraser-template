/*
 * PCDUModel.h
 *
 *  Created on: Dec 29, 2016
 *      Author: Annika Ofenloch
 */

#ifndef MODEL_2_MODEL_2_H_
#define MODEL_2_MODEL_2_H_

#include <fstream>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <zmq.hpp>

#include "communication/zhelpers.hpp"
#include "communication/Subscriber.h"
#include "communication/Publisher.h"
#include "communication/Dealer.h"
#include "interfaces/IModel.h"
#include "interfaces/IPersist.h"
#include "data-types/Field.h"

#include "resources/idl/event_generated.h"

class Model2: public virtual IModel,
		public virtual IPersist {
public:
	Model2(std::string name, std::string description);
	virtual ~Model2();

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
	std::string mData;

	// Event Serialiazation
	flatbuffers::FlatBufferBuilder mFbb;
	flatbuffers::Offset<event::Event> mEventOffset;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& archive, const unsigned int) {
	}
};

//BOOST_CLASS_VERSION(Model2, 0);
//BOOST_CLASS_IMPLEMENTATION(Model2, boost::serialization::object_serializable)

#endif /* MODEL_2_MODEL_2_H_ */
