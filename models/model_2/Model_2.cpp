/*
 * Model2.cpp
 *
 *  Created on: Dec 29, 2016
 *      Author: Annika Ofenloch
 */

#include <iostream>
#include "Model_2.h"

static const char BREAKPNTS_PATH[] = "models/model_2/savepoints/";
static const char FILE_EXTENTION[] = "_savefile_model_2.xml";

Model2::Model2(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mReceivedEvent(NULL), mCurrentSimTime(
				0) {

	mRun = this->prepare();
}

Model2::~Model2() {
}

void Model2::configure(std::string filename) {
	// Load config-file
	// Set values for the fields
	this->loadState(filename);

	// Set or calculate other parameters ...
}

bool Model2::prepare() {
	mSubscriber.setOwnershipName(mName);

//	boost::filesystem::path dir1(BREAKPNTS_PATH);
//	if (!boost::filesystem::exists(dir1)) {
//		boost::filesystem::create_directory(dir1);
//		std::cout << "Create savepoints-directory for Model2" << "\n";
//	}
//
//	boost::filesystem::path dir2(CONFIG_DIR);
//	if (!boost::filesystem::exists(dir2)) {
//		boost::filesystem::create_directory(dir2);
//		std::cout << "Create config-directory for Model2" << "\n";
//	}

	if (!mPublisher.bindSocket(mDealer.getPortNumFrom(mName))) {
		return false;
	}

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("simulation_model"),
			mDealer.getPortNumFrom("simulation_model"))) {
		return false;
	}

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("event_queue_1"),
			mDealer.getPortNumFrom("event_queue_1"))) {
		return false;
	}

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("model_1"),
			mDealer.getPortNumFrom("model_1"))) {
		return false;
	}

	mSubscriber.subscribeTo("SubsequentEvent");
	mSubscriber.subscribeTo("Restore");
	mSubscriber.subscribeTo("Store");
	mSubscriber.subscribeTo("End");
	mSubscriber.subscribeTo("PCDUCommand");
	mSubscriber.subscribeTo("CreateDefaultConfigFiles");
	mSubscriber.subscribeTo("Configure");

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

void Model2::run() {
	while (mRun) {
		mSubscriber.receiveEvent();
		this->handleEvent();
	}
}

void Model2::handleEvent() {
	mReceivedEvent = event::GetEvent(mSubscriber.getEventBuffer());
	mEventName = mReceivedEvent->name()->str();
	mCurrentSimTime = mReceivedEvent->timestamp();
	mData = mReceivedEvent->data_as_String()->str();

	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (mEventName == "SubsequentEvent") {
		mEventOffset = event::CreateEvent(mFbb, mFbb.CreateString("ReturnEvent"), mCurrentSimTime);
		mFbb.Finish(mEventOffset);
		this->mPublisher.publishEvent("ReturnEvent", mFbb.GetBufferPointer(), mFbb.GetSize());
	}

	else if (mEventName == "CreateDefaultConfigFiles") {
		this->saveState(std::string(mData.begin(), mData.end()) + mName + ".config");
	}

	else if (mEventName == "Configure") {
		this->configure(std::string(mData.begin(), mData.end()) + mName + ".config");
	}

	else if (mEventName == "Store" || mEventName == "Restore") {
		std::string filePath = BREAKPNTS_PATH
				+ std::to_string(mReceivedEvent->timestamp()) + FILE_EXTENTION;

		if (mEventName == "Store") {
			std::cout << "Store events from Queue" << std::endl;
			this->saveState(filePath);

		} else {
			std::cout << "Restore events from Queue" << std::endl;
			this->loadState(filePath);
		}
	}

	else if (mEventName == "End") {
		mRun = false;
	}
}

void Model2::saveState(std::string filePath) {
	// Store states
	std::ofstream ofs(filePath);
	boost::archive::xml_oarchive oa(ofs, boost::archive::no_header);
	try {
		oa << boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ": Archive Exception during serializing:" << std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();
}

void Model2::loadState(std::string filePath) {
	// Restore states
	std::ifstream ifs(filePath);
	boost::archive::xml_iarchive ia(ifs, boost::archive::no_header);
	try {
		ia >> boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ": Archive Exception during deserializing:" << std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();
}
