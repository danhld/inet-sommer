#include "sampleApp.h"
#include "SampleUpperCtrlInfo_m.h"

Define_Module(sampleApp);

void sampleApp::initialize(int aStage)
{
	TrafGen::initialize(aStage);

	if(0 == aStage){
		mLowergateIn            = findGate("lowergateIn");
		mLowergateOut           = findGate("lowergateOut");

		// if needed, change the current traffic pattern
		//mCurrentTrafficPattern = 0;
		// set parameters for the traffic generator
		//setParams(mCurrentTrafficPattern);

		mNumTrafficMsgs = 0;

		msgSentVec.setName("Message sent");
	} 
}

void sampleApp::finish()
{
	recordScalar("trafficSent", mNumTrafficMsgs);
}

void sampleApp::handleLowerMsg(cMessage* apMsg)
{
	ev << "message arrived at application." << endl;
	delete apMsg;
}

void sampleApp::handleSelfMsg(cMessage *apMsg)
{
	TrafGen::handleSelfMsg(apMsg);
}

/** this function has to be redefined in every application derived from the 
	TrafGen class.
	Its purpose is to translate the destination (given, for example, as "host[5]")
	to a valid address (MAC, IP, ...) that can be understood by the next lower 
	layer.
	It also constructs an appropriate control info block that might be needed by 
	the lower layer to process the message.
	In the example, the messages are sent directly to a mac 802.11 layer, address 
	and control info are selected accordingly.
*/
void sampleApp::SendTraf(cPacket* apMsg, const char* apDest)
{
	SampleUpperCtrlInfo *control_info = new SampleUpperCtrlInfo();

	control_info->setDestName(apDest);
	apMsg->setControlInfo(control_info);

	apMsg->setBitLength(PacketSize()*8);
	mNumTrafficMsgs++;
	msgSentVec.record(simTime());
	send(apMsg, mLowergateOut);

}
