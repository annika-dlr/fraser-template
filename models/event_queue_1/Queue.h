/*
 * Queue.h
 *
 *  Created on: Jan 3, 2017
 *      Author: Annika Ofenloch
 */

#ifndef EVENT_QUEUE_1_QUEUE_H_
#define EVENT_QUEUE_1_QUEUE_H_

#include <fstream>
#include <functional>
#include <boost/serialization/vector.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <zmq.hpp>

#include "communication/Dealer.h"
#include "communication/Publisher.h"
#include "communication/Subscriber.h"
#include "data-types/EventSet.h"
#include "communication/zhelpers.hpp"
#include "interfaces/IModel.h"
#include "interfaces/IPersist.h"
#include "interfaces/IQueue.h"
#include "scheduler/Scheduler.h"

#include "resources/idl/event_generated.h"

class Queue: public virtual IModel,
		public virtual IPersist,
		public virtual IQueue {

public:
	Queue(std::string name, std::string description);
	virtual ~Queue();

	// IModel
	virtual void configure(std::string filename) override;
	virtual bool prepare() override;
	virtual void run() override;
	virtual std::string getName() const override {
		return mName;
	}
	virtual std::string getDescription() const override {
		return mDescription;
	}

	// IPersist
	virtual void store(std::string filename) override;
	virtual void restore(std::string filename) override;

private:
	void handleEvent();
	void setDefaultEvents();

	// IQueue
	virtual void updateEvents() override;
	// TODO: Change to std::vector< flatbuffers::Offset<event::Event> >
	EventSet mEventSet;

	std::string mName;
	std::string mDescription;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& archive, const unsigned int) {
		archive & boost::serialization::make_nvp("EventSet", mEventSet);
	}

	// Subscriber & Publisher
	zmq::context_t mCtx;
	Subscriber mSubscriber;
	Publisher mPublisher;
	Dealer mDealer;

	bool mRun;
	const event::Event* mReceivedEvent;
	std::string mEventName;
	std::string mData;

	Scheduler mScheduler;
	uint64_t mCurrentSimTime;

	// Serialization
	flatbuffers::FlatBufferBuilder mFbb;
	flatbuffers::Offset<event::Event> mEventOffset;
};

#endif /* EVENT_QUEUE_1_QUEUE_H_ */
