/** 
 * @short This module implements a sample routing protocol.
 * You can use it as a starting point for your own protocol.
 * 
 * Important: Please rename all occurrences of "sample" 
 * to something meaningful in the context of your protocol!
 * 
 *  @author Isabel Dietrich, isabel.dietrich@informatik.uni-erlangen.de
*/

#include "sampleRoutingProtocol.h"

Define_Module( sampleRoutingProtocol );

/////////////////////////////// PUBLIC ///////////////////////////////////////

//============================= LIFECYCLE ===================================
void sampleRoutingProtocol::initialize(int aStage)
{
	cSimpleModule::initialize(aStage);
	if (0 == aStage)
	{		
		mUppergateIn  = findGate("uppergateIn");
		mUppergateOut = findGate("uppergateOut");
		mLowergateIn  = findGate("ifIn");
		mLowergateOut = findGate("ifOut");

		mHeaderLength = par("headerLengthByte");

		myAddr = getParentModule()->getId();
	} 
	else if (1 == aStage) 
	{

	}
}

void sampleRoutingProtocol::finish()
{
	// hier können Statistiken aufgezeichnet werden
	// und dynamischer Speicher aufgeräumt werden
}

//============================= OPERATIONS ===================================
/** 
 * Function called whenever a message arrives at the module
 */
void sampleRoutingProtocol::handleMessage(cMessage* apMsg)
{
	if (mUppergateIn == apMsg->getArrivalGateId())
	{
		HandleUpperMsg(apMsg);
	}
	else if (apMsg->isSelfMessage())
	{
		HandleSelfMsg(apMsg);
	}
	else
	{
		HandleLowerMsg(apMsg);
	}
}

/////////////////////////////// PRIVATE  ///////////////////////////////////

//============================= OPERATIONS ===================================
/**
 * HandleUpperMsg reacts to all messages arriving from gate uppergateIn
 */
void sampleRoutingProtocol::HandleUpperMsg(cMessage* apMsg)
{
	// hier kommen App-Pakete an, encapsMsg() aufrufen
	SampleRoutingPkt *net = encapsMsg(apMsg);

	// ab jetzt liefert pkt->getDestAddress() die Zieladresse, 
	// man kann hier also z.B. die Route Discovery anstossen
	// das sampleRoutingProtocol tut hier nichts.

	// wenn die Route bekannt ist, kann man das Paket dann versenden:
	SendDown(net);

}

/**
 * HandleLowerMsg reacts to all messages arriving from gate lowergateIn 
 */
void sampleRoutingProtocol::HandleLowerMsg(cMessage* apMsg)
{
	SendUp(check_and_cast<cPacket*>(apMsg)->decapsulate());
	delete apMsg;

}

/**
 * HandleSelfMsg can react to all kinds of timers
 */
void sampleRoutingProtocol::HandleSelfMsg(cMessage* apMsg)
{
	switch (apMsg->getKind())
	{
	}
}

/**
 * Encapsulates the received application packet into a SampleRoutingPkt
 * and sets all needed header fields.
 */
SampleRoutingPkt *sampleRoutingProtocol::encapsMsg(cMessage* apMsg)
{

	unsigned int destAddr;
	SampleUpperCtrlInfo* control_info = check_and_cast<SampleUpperCtrlInfo *>(apMsg->removeControlInfo());

	SampleRoutingPkt* pkt = new SampleRoutingPkt(apMsg->getName());
	pkt->setBitLength(mHeaderLength);        // headerLength, including final CRC-field

	if(!getParentModule()->getParentModule()->getModuleByRelativePath(control_info->getDestName()))
		error("destination host does not exist!");

	/** get the IP-Address of the destination node **/
	destAddr = getParentModule()->getParentModule()->getModuleByRelativePath(control_info->getDestName())->getId();

	pkt->setDestAddress(destAddr);
	pkt->setSrcAddress(myAddr);
	pkt->setCreationTime(simTime());

	pkt->encapsulate(check_and_cast<cPacket*>(apMsg));

	delete control_info;

	return pkt;
}


/**
 * Function SendDown: takes a message, looks up the destination address on the lower layer
 * and sends it down to the next layer
 */ 
void sampleRoutingProtocol::SendDown(cMessage* apMsg)
{
	Ieee802Ctrl *control_info = new Ieee802Ctrl();
	MACAddress mac_dest;
	int destAddr = dynamic_cast<SampleRoutingPkt*>(apMsg)->getDestAddress();

	// if your protocol has a broadcast address, check for it here
	if(destAddr == -1)
		mac_dest = MACAddress::BROADCAST_ADDRESS;
	else {
		if(simulation.getModule(destAddr) != NULL){
			mac_dest = check_and_cast<IInterfaceTable *>(simulation.getModule(destAddr)->getModuleByRelativePath("interfaceTable"))->getInterfaceByName("wlan")->getMacAddress();
		}
		else
			error("cannot find a host with the given address");
	}		

	control_info->setDest(mac_dest);
	apMsg->setControlInfo(control_info);

	ev << "sending down " << apMsg << "\n";
	send(apMsg, mLowergateOut);
}

/**
 * Function SendUp: takes a cMessage and sends it to the upper layer
 */ 
void sampleRoutingProtocol::SendUp(cMessage* apMsg)
{
	ev << "sending up " << apMsg << "\n";
	send(apMsg, mUppergateOut);
}
