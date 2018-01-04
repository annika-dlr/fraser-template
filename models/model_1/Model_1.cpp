/*
 * Model1.cpp
 *
 *  Created on: Dec 29, 2016
 *      Author: Annika Ofenloch
 */

#include <iostream>
#include "Model_1.h"

static const char BREAKPNTS_PATH[] = "models/model_1/savepoints/";
static const char FILE_EXTENTION[] = "_savefile_model_1.xml";

Model1::Model1(std::string name, std::string description) :
		mName(name), mDescription(description), mCtx(1), mSubscriber(mCtx), mPublisher(
				mCtx), mDealer(mCtx, mName), mReceivedEvent(NULL), mCurrentSimTime(
				0) {

	mRun = this->prepare();
}

Model1::~Model1() {
}

void Model1::configure(std::string configPath) {
	// Load config-file
	// Set values for the fields
	this->loadState(configPath);

	// Set or calculate other parameters ...
}

bool Model1::prepare() {
	mSubscriber.setOwnershipName(mName);

//	boost::filesystem::path dir1(BREAKPNTS_PATH);
//
//	if (!boost::filesystem::exists(dir1)) {
//		boost::filesystem::create_directory(dir1);
//		std::cout << "Create savepoints-directory for Model1" << "\n";
//	}
//
//	boost::filesystem::path dir2(CONFIG_DIR);
//	if (!boost::filesystem::exists(dir2)) {
//		boost::filesystem::create_directory(dir2);
//		std::cout << "Create config-directory for Model1" << "\n";
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

	if (!mSubscriber.connectToPub(mDealer.getIPFrom("model_2"),
			mDealer.getPortNumFrom("model_2"))) {
		return false;
	}

	mSubscriber.subscribeTo("FirstEvent");
	mSubscriber.subscribeTo("ReturnEvent");
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

void Model1::run() {
	while (mRun) {
		mSubscriber.receiveEvent();
		this->handleEvent();
	}
}

void Model1::handleEvent() {
	mReceivedEvent = event::GetEvent(mSubscriber.getEventBuffer());
	mEventName = mReceivedEvent->name()->str();
	mCurrentSimTime = mReceivedEvent->timestamp();
	mData = mReceivedEvent->data_as_String()->str();

	mRun = !foundCriticalSimCycle(mCurrentSimTime);

	if (mEventName == "FirstEvent") {
		mEventOffset = event::CreateEvent(mFbb,
				mFbb.CreateString("SubsequentEvent"), mCurrentSimTime);
		mFbb.Finish(mEventOffset);
		this->mPublisher.publishEvent("SubsequentEvent",
				mFbb.GetBufferPointer(), mFbb.GetSize());
	}

	else if (mEventName == "ReturnEvent") {

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

void Model1::saveState(std::string filePath) {
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

void Model1::loadState(std::string filePath) {
	// Restore states
	std::ifstream ifs(filePath);
	boost::archive::xml_iarchive ia(ifs, boost::archive::no_header);
	try {
		ia >> boost::serialization::make_nvp("FieldSet", *this);

	} catch (boost::archive::archive_exception& ex) {
		std::cout << mName << ":Archive Exception during deserializing:" << std::endl;
		std::cout << ex.what() << std::endl;
	}

	mRun = mSubscriber.synchronizeSub();
}
