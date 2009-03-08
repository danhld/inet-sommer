//
// Copyright (C) 2006 Autonomic Networking Group,
// Department of Computer Science 7, University of Erlangen, Germany
//
// Author: Isabel Dietrich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "applications/trafgen/DymoTestApp.h"
#include "networklayer/dymo/AppPkt_m.h"

//#define EV (ev.isDisabled()) ? std::cout : ev ==> EV is now part of <omnetpp.h>

Define_Module(DymoTestApp);

void DymoTestApp::initialize(int aStage)
{
	TrafGen::initialize(aStage);

	if(0 == aStage){
   	    mLowergateIn            = findGate("lowergate$i");
		mLowergateOut           = findGate("lowergate$o");

		// if needed, change the current traffic pattern
		//mCurrentTrafficPattern = 0;
		// set parameters for the traffic generator
		//setParams(mCurrentTrafficPattern);

		mNumTrafficMsgs = 0;
		mNumTrafficMsgRcvd = 0;
		mNumTrafficMsgNotDelivered = 0;

	}
}

void DymoTestApp::finish()
{
	recordScalar("trafficSent", mNumTrafficMsgs);
	recordScalar("trafficRcvd", mNumTrafficMsgRcvd);
	recordScalar("notDeliveredMsg", mNumTrafficMsgNotDelivered);
	TrafGen::finish();
}

void DymoTestApp::handleLowerMsg(cPacket* apMsg)
{
	if(dynamic_cast<ICMPMessage*>(apMsg) == NULL)
		mNumTrafficMsgRcvd++;
	else
		mNumTrafficMsgNotDelivered++;
	delete apMsg;}

void DymoTestApp::handleSelfMsg(cMessage *apMsg)
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
void DymoTestApp::SendTraf(cPacket* apMsg, const char* apDest)
{
	using namespace DYMOnet;

	AppPkt *control_info = new AppPkt();

	control_info->setDestName(apDest);
	apMsg->setControlInfo(control_info);

	apMsg->setBitLength(PacketSize()*8);
	mNumTrafficMsgs++;
	send(apMsg, mLowergateOut);

}
