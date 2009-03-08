/*
 *  Copyright (C) 2005 Mohamed Louizi
 *  Copyright (C) 2006,2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "networklayer/dymo/DYMO.h"

Define_Module( DYMOnet__DYMO );

namespace {
	const int DYMO_RM_HEADER_LENGTH = 13; /**< length (in bytes) of a DYMO RM header */
	const int DYMO_RBLOCK_LENGTH = 10; /**< length (in bytes) of one DYMO RBlock */
	const int DYMO_RERR_HEADER_LENGTH = 4; /**< length (in bytes) of a DYMO RERR header */
	const int DYMO_UBLOCK_LENGTH = 8; /**< length (in bytes) of one DYMO UBlock */
	const double MAXJITTER = 0.001; /**< all messages sent to a lower layer are delayed by 0..MAXJITTER seconds (draft-ietf-manet-jitter-01) */
	const IPAddress LL_MANET_ROUTERS = IPAddress(-1); /**< Link-local multicast address of all MANET routers (TBD) */
}

void DYMOnet__DYMO::initialize(int aStage) {
	cSimpleModule::initialize(aStage);

	if (0 == aStage) {
		ownSeqNumLossTimeout = new DYMO_Timer(this, "OwnSeqNumLossTimeout");
		WATCH_PTR(ownSeqNumLossTimeout);
		ownSeqNumLossTimeoutMax = new DYMO_Timer(this, "OwnSeqNumLossTimeoutMax");
		WATCH_PTR(ownSeqNumLossTimeoutMax);
		//TODO assume SeqNum loss when starting up?

		mUppergateIn  = findGate("uppergate$i");
		mUppergateOut = findGate("uppergate$o");
		mLowergateIn[0]  = findGate("ifIn", 0);
		mLowergateOut[0] = findGate("ifOut", 0);
		mLowergateIn[1]  = findGate("ifIn", 1);
		mLowergateOut[1] = findGate("ifOut", 1);

		statsTrafficSent = 0; /**< number of injected Application-layer messages */
		statsTrafficRcvd = 0; /**< number of received Application-layer messages (for this node) */
		statsTrafficFwd = 0; /**< number of Application-layer messages sent on behalf of other nodes */

		statsRREQSent = 0; /**< number of generated DYMO RREQs */
		statsRREPSent = 0; /**< number of generated DYMO RREPs */
		statsRERRSent = 0; /**< number of generated DYMO RERRs */

		statsRREQRcvd = 0; /**< number of consumed DYMO RREQs */
		statsRREPRcvd = 0; /**< number of consumed DYMO RREPs */
		statsRERRRcvd = 0; /**< number of consumed DYMO RERRs */

		statsRREQFwd = 0; /**< number of forwarded (and processed) DYMO RREQs */
		statsRREPFwd = 0; /**< number of forwarded (and processed) DYMO RREPs */
		statsRERRFwd = 0; /**< number of forwarded (and processed) DYMO RERRs */

		statsDYMORcvd = 0; /**< number of observed DYMO messages */

		discoveryLatency = 0;
		disSamples = 0;
		dataLatency = 0;
		dataSamples = 0;

		ownSeqNum = 1;

		queuedElements = new cQueue();
		// svkers
		//WATCH_VECTOR( queuedRREQElements );

		discoveryDelayVec.setName("Discovery delay");
		dataDelayVec.setName("Data delay");
		dataDelayPerHop.setName("Data delay per hop");
		dataSourceVec.setName("Data source");
		dataNumHops.setName("Data hops");
		//dataLoadVec.setName("Data load");
		//controlLoadVec.setName("Control load");

		RESPONSIBLE_ADDRESSES_PREFIX=par("RESPONSIBLE_ADDRESSES_PREFIX");
		DYMO_INTERFACES=par("DYMO_INTERFACES");
		AUTOASSIGN_ADDRESS_BASE=IPAddress(par("AUTOASSIGN_ADDRESS_BASE").stringValue());
		ROUTE_AGE_MIN_TIMEOUT=par("ROUTE_AGE_MIN_TIMEOUT");
		ROUTE_AGE_MAX_TIMEOUT=par("ROUTE_AGE_MAX_TIMEOUT");
		ROUTE_NEW_TIMEOUT=par("ROUTE_NEW_TIMEOUT");
		ROUTE_USED_TIMEOUT=par("ROUTE_USED_TIMEOUT");
		ROUTE_DELETE_TIMEOUT=par("ROUTE_DELETE_TIMEOUT");
		MIN_HOPLIMIT=par("MIN_HOPLIMIT");
		MAX_HOPLIMIT=par("MAX_HOPLIMIT");
		RREQ_RATE_LIMIT=par("RREQ_RATE_LIMIT");
		RREQ_BURST_LIMIT=par("RREQ_BURST_LIMIT");
		RREQ_WAIT_TIME=par("RREQ_WAIT_TIME");
		RREQ_TRIES=par("RREQ_TRIES");

		// -v- DYMOnet needs AUTOASSIGN_ADDRESS_BASE to be set to 0.0.0.0
		if (AUTOASSIGN_ADDRESS_BASE.getInt() != 0) error("DYMOnet needs AUTOASSIGN_ADDRESS_BASE to be set to 0.0.0.0");
		// -^-

		myAddr = AUTOASSIGN_ADDRESS_BASE.getInt() + uint32(getParentModule()->getId());

		rateLimiterRREQ = new DYMO_TokenBucket(RREQ_RATE_LIMIT, RREQ_BURST_LIMIT, simTime());

	}
	if (2 == aStage) {
		dymo_routingTable = new DYMO_RoutingTable(getParentModule(), IPAddress(myAddr), DYMO_INTERFACES, LL_MANET_ROUTERS);
		WATCH_PTR(dymo_routingTable);

		outstandingRREQList.delAll();
		WATCH_OBJ(outstandingRREQList);
	}
}

void DYMOnet__DYMO::finish() {
	recordScalar("DYMO_TrafficSent", statsTrafficSent);
	recordScalar("DYMO_TrafficRcvd", statsTrafficRcvd);
	recordScalar("DYMO_TrafficFwd", statsTrafficFwd);

	recordScalar("DYMO_RREQSent", statsRREQSent);
	recordScalar("DYMO_RREPSent", statsRREPSent);
	recordScalar("DYMO_RERRSent", statsRERRSent);

	recordScalar("DYMO_RREQRcvd", statsRREQRcvd);
	recordScalar("DYMO_RREPRcvd", statsRREPRcvd);
	recordScalar("DYMO_RERRRcvd", statsRERRRcvd);

	recordScalar("DYMO_RREQFwd", statsRREQFwd);
	recordScalar("DYMO_RREPFwd", statsRREPFwd);
	recordScalar("DYMO_RERRFwd", statsRERRFwd);

	recordScalar("DYMO_DYMORcvd", statsDYMORcvd);

	if(discoveryLatency > 0 && disSamples > 0)
		recordScalar("discovery latency", discoveryLatency/disSamples);
	if(dataLatency > 0 && dataSamples > 0)
		recordScalar("data latency", dataLatency/dataSamples);

	delete queuedElements;

	delete dymo_routingTable;
	dymo_routingTable = 0;

	outstandingRREQList.delAll();

	delete ownSeqNumLossTimeout;
	ownSeqNumLossTimeout = 0;
	delete ownSeqNumLossTimeoutMax;
	ownSeqNumLossTimeoutMax = 0;

	delete rateLimiterRREQ;
	rateLimiterRREQ = 0;
}

void DYMOnet__DYMO::handleMessage(cMessage* apMsg) {
	if (mUppergateIn == apMsg->getArrivalGateId()) {
		handleUpperMsg(check_and_cast<cPacket*>(apMsg));
	}
	else if (apMsg->isSelfMessage()) {
		handleSelfMsg(apMsg);
	}
	else {
		cPacket* apPkt = check_and_cast<cPacket*>(apMsg);
		handleLowerMsg(apPkt);
	}
}

void DYMOnet__DYMO::handleUpperMsg(cPacket* apMsg) {
	DYMO_Packet * pkt = encapsMsg(apMsg);

	sendDataPacket(pkt);
}

void DYMOnet__DYMO::sendDataPacket(DYMO_Packet *packet) {
	Enter_Method("onOutboundDataPacket(%s)", packet->getName());

	//IPControlInfo* controlInfo = check_and_cast<IPControlInfo*>(packet->getControlInfo());
	IPAddress destAddr = packet->getDestAddress();
	int TargetSeqNum=0;
	int TargetHopCount=0;

	// skip route discovery if this packet is for us
	if(destAddr.getInt() == myAddr)
	{
		ev << "onOutboundDataPacket destAddr == myAddr (" << myAddr << ")\n";
		checkAndSendQueuedPkts(destAddr.getInt(), 32, destAddr.getInt());

		// -v- DYMOnet also passes packet up
		sendUp(packet->decapsulate());
		delete packet;
		// -^-

		return;
	}

	// look up routing table entry for packet destination
	DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(destAddr);
	if (entry) {
		// if a valid route exists, signal the queue to send all packets stored for this destination
		if (!entry->routeBroken) {

			//TODO: mark route as used when forwarding data packets? Draft says yes, but as we are using Route Timeout to "detect" link breaks, this seems to be a bad idea
			// update routes to destination
			//updateRouteLifetimes(destAddr.getInt());

			// send queued packets
			checkAndSendQueuedPkts(entry->routeAddress.getInt(), entry->routePrefix, (entry->routeNextHopAddress).getInt());

			// -v- DYMOnet also passes packet down
			packet->setNextHopAddress((entry->routeNextHopAddress).getInt());
			//discoveryDelayVec.record(0);
			//disSamples++;
			statsTrafficSent++;
			sendDown(packet, packet->getNextHopAddress());
			// -^-

			return;
		}

		TargetSeqNum=entry->routeSeqNum;
		TargetHopCount=entry->routeDist;
	}

	// no route in the table found -> route discovery (if none is already underway)
	if (!outstandingRREQList.getByDestAddr(destAddr.getInt(), 32)) {
		sendRREQ(destAddr.getInt(), MIN_HOPLIMIT, TargetSeqNum, TargetHopCount);

		/** queue the RREQ in a list and schedule a timeout in order to resend the RREQ when no RREP is received **/
		DYMO_OutstandingRREQ* outstandingRREQ = new DYMO_OutstandingRREQ;
		outstandingRREQ->tries = 1;
		outstandingRREQ->wait_time = new DYMO_Timer(this, "RREQ wait time");
		outstandingRREQ->wait_time->start(RREQ_WAIT_TIME);
		outstandingRREQ->destAddr = destAddr.getInt();
		outstandingRREQ->creationTime = simTime();
		outstandingRREQList.add(outstandingRREQ);

		// -v- DYMOnet also queues the packet
		QueueElement * qElement = new QueueElement(ownSeqNum, packet->getDestAddress(), myAddr, packet);
		queuedElements->insert(qElement);
		// -^-

	}
}

void DYMOnet__DYMO::handleLowerPacket(DYMO_Packet *datagram) {
	Enter_Method("onInboundDataPacket(%s)", datagram->getName());

	IPAddress srcAddr = datagram->getSrcAddress();
	IPAddress destAddr = datagram->getDestAddress();
	int TargetSeqNum=0;

	// find out if this is a DYMO packet rather than actual payload
	bool isDymoPacket = false;
	//if (datagram->getTransportProtocol() == IP_PROT_UDP) {
	//	UDPPacket* packet = check_and_cast<UDPPacket*>(datagram->getEncapsulatedMsg());
	//	if (packet->getDestinationPort() == UDPPort) isDymoPacket = true;
	//}

	// if this is a DYMO packet, don't act upon it and just pass it through
	if (isDymoPacket) return;

	ev << "inbound data packet signalled" << endl;

	// if this is for us, record statistics
	if (destAddr.getInt() == myAddr) {
		dataDelayVec.record(simTime() - datagram->getCreationTime());
		dataLatency += SIMTIME_DBL(simTime() - datagram->getCreationTime());
		dataSamples++;

		// -v- DYMOnet also records additional statistics and passes packet up
		DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(srcAddr);
		int hopCount = 0;
		if(entry) {
			hopCount = entry->routeDist + 1;
			dataNumHops.record(hopCount);
			// record latency per hop (useful to put under Akaroa control)
			dataDelayPerHop.record((simTime() - datagram->getCreationTime())/hopCount);
		}
		dataSourceVec.record(datagram->getSrcAddress());
		statsTrafficRcvd++;
		sendUp(datagram->decapsulate());
		delete datagram;
		// -^-

		return;
	}

	// if this is a broadcast or multicast, everything is fine
	if ((destAddr == IPAddress::ALLONES_ADDRESS) || (destAddr.isMulticast())) return;

	// FIXME: If ownSeqNumLossTimeout->isRunning(): discard packet, send RERR, reset ownSeqNumLossTimeout to ROUTE_DELETE_TIMEOUT, but updateRouteLifetimes(?)

	// look up routing table entry for packet destination
	DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(destAddr);
	if (entry) {
		// if a valid route exists, everything's alright
		if (!entry->routeBroken) {

			// update routes to source
			updateRouteLifetimes(srcAddr.getInt());

			// -v- DYMOnet also passes packet to next hop
			unsigned int nextHopAddr = (entry->routeNextHopAddress).getInt();
			datagram->setNextHopAddress(nextHopAddr);
			statsTrafficFwd++;
			sendDown(datagram, nextHopAddr);
			// -^-

			return;
		}

		TargetSeqNum=entry->routeSeqNum;
	}

	// no route in the table found -> send RERR
	sendRERR(destAddr.getInt(), TargetSeqNum);

	// the packet will be discarded by the network layer, so we'll leave it at that
	dymo_routingTable->maintainAssociatedRoutingTable();

	// -v- DYMOnet also deletes packet
	delete datagram;
	// -^-
}

void DYMOnet__DYMO::handleLowerMsg(cPacket* apMsg) {
	// detach and free associated ControlInfo object
	delete apMsg->removeControlInfo();

	/**
	 * check the type of received message
	 1) Routing Message: RREQ or RREP
	 2) Error Message: RERR
	 3) Unsupported Message: UERR
	 4) Data Message
	 **/
	if(dynamic_cast<DYMO_RM*>(apMsg)) handleLowerRM(dynamic_cast<DYMO_RM*>(apMsg));
	else if(dynamic_cast<DYMO_RERR*>(apMsg)) handleLowerRERR(dynamic_cast<DYMO_RERR*>(apMsg));
	else if(dynamic_cast<DYMO_UERR*>(apMsg)) handleLowerUERR(dynamic_cast<DYMO_UERR*>(apMsg));
	else if(dynamic_cast<DYMO_Packet*>(apMsg)) handleLowerPacket(dynamic_cast<DYMO_Packet*>(apMsg));
	else error("cannot cast message to DYMO Packet");
}

void DYMOnet__DYMO::handleLowerRM(DYMO_RM *routingMsg) {
	/** message is a routing message **/
	ev << "received message is a routing message" << endl;

	statsDYMORcvd++;

	/** routing message  preprocessing and updating routes from routing blocks **/
	if(updateRoutes(routingMsg) == NULL) {
		ev << "dropping received message" << endl;
		delete routingMsg;
		return;
	}

	/**
	 * received message is a routing message.
	 * check if the node is the destination
	 * 1) YES - if the RM is a RREQ, then send a RREP to source
	 * 2) NO - send message down to next node.
	 **/
	if(myAddr == routingMsg->getTargetNode().getAddress()) {
		handleLowerRMForMe(routingMsg);
		return;
	} else {
		handleLowerRMForRelay(routingMsg);
		return;
	}


}

uint32_t DYMOnet__DYMO::getNextHopAddress(DYMO_RM *routingMsg) {
	if (routingMsg->getAdditionalNodes().size() > 0) {
		return routingMsg->getAdditionalNodes().back().getAddress();
	} else {
		return routingMsg->getOrigNode().getAddress();
	}
}

InterfaceEntry* DYMOnet__DYMO::getNextHopInterface(DYMO_PacketBBMessage* pkt) {

	if (!pkt) error("getNextHopInterface called with NULL packet");

	int addr = 0;

	DYMO_RM* drm = dynamic_cast<DYMO_RM*>(pkt);
	if (drm) {
		addr = getNextHopAddress(drm);
	}

	char buf[30];
	sprintf(buf,"host[%d].interfaceTable", addr);

	if(!getParentModule()->getParentModule()->getModuleByRelativePath(buf)) return 0;

	return check_and_cast<IInterfaceTable *>(getParentModule()->getParentModule()->getModuleByRelativePath(buf))->getInterfaceByName("wlan");
}

void DYMOnet__DYMO::handleLowerRMForMe(DYMO_RM *routingMsg) {
	/** current node is the target **/
	if(dynamic_cast<DYMO_RREQ*>(routingMsg)) {
		/** received message is a RREQ -> send a RREP to source **/
		sendReply(routingMsg->getOrigNode().getAddress(), (routingMsg->getTargetNode().hasSeqNum() ? routingMsg->getTargetNode().getSeqNum() : 0));
		statsRREQRcvd++;
		delete routingMsg;
	}
	else if(dynamic_cast<DYMO_RREP*>(routingMsg)) {
		/** received message is a RREP **/
		statsRREPRcvd++;

		// signal the queue to dequeue waiting messages for this destination
		checkAndSendQueuedPkts(routingMsg->getOrigNode().getAddress(), (routingMsg->getOrigNode().hasPrefix() ? routingMsg->getOrigNode().getPrefix() : 32), getNextHopAddress(routingMsg));

		delete routingMsg;
	}
	else error("received unknown dymo message");
}

void DYMOnet__DYMO::handleLowerRMForRelay(DYMO_RM *routingMsg) {

	/** current node is not the message destination -> find route to destination **/
	ev << "current node is not the message destination -> find route to destination" << endl;

	unsigned int targetAddr = routingMsg->getTargetNode().getAddress();
	unsigned int targetSeqNum = 0;

	// stores route entry of route to destination if a route exists, 0 otherwise
	DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(targetAddr);
	if(entry) {
		targetSeqNum = entry->routeSeqNum;

		//TODO: mark route as used when forwarding DYMO packets?
		//entry->routeUsed.start(ROUTE_USED_TIMEOUT);
		//entry->routeDelete.cancel();

		if(entry->routeBroken) entry = 0;
	}

	/** received routing message is an RREP and no routing entry was found **/
	if(dynamic_cast<DYMO_RREP*>(routingMsg) && (!entry)) {
		/* do nothing, just drop the RREP */
		ev << "no route to destination of RREP was found. Sending RERR and dropping message." << endl;
		sendRERR(targetAddr, targetSeqNum);
		delete routingMsg;
		return;
	}

	// check if received message is a RREQ and a routing entry was found
	if (dynamic_cast<DYMO_RREQ*>(routingMsg) && (entry) && (routingMsg->getTargetNode().hasSeqNum()) && (!seqNumIsFresher(routingMsg->getTargetNode().getSeqNum(), entry->routeSeqNum))) {
		// yes, we can. Do intermediate DYMO router RREP creation
		ev << "route to destination of RREQ was found. Sending intermediate DYMO router RREP" << endl;
		sendReplyAsIntermediateRouter(routingMsg->getOrigNode(), routingMsg->getTargetNode(), entry);
		statsRREQRcvd++;
		delete routingMsg;
		return;
	}

	/** check whether a RREQ was sent to discover route to destination **/
	ev << "received message is a RREQ" << endl;
	ev << "trying to discover route to node " << targetAddr << endl;

	/** increment distance metric of existing AddressBlocks */
	std::vector<DYMO_AddressBlock> additional_nodes = routingMsg->getAdditionalNodes();
	std::vector<DYMO_AddressBlock> additional_nodes_to_relay;
	if (routingMsg->getOrigNode().hasDist() && (routingMsg->getOrigNode().getDist() >= 0xFF - 1)) {
		ev << "passing on this message would overflow OrigNode.Dist -> dropping message" << endl;
		delete routingMsg;
		return;
	}
	routingMsg->getOrigNode().incrementDistIfAvailable();
	for (unsigned int i = 0; i < additional_nodes.size(); i++) {
		if (additional_nodes[i].hasDist() && (additional_nodes[i].getDist() >= 0xFF - 1)) {
			ev << "passing on additionalNode would overflow OrigNode.Dist -> dropping additionalNode" << endl;
			continue;
		}
		additional_nodes[i].incrementDistIfAvailable();
		additional_nodes_to_relay.push_back(additional_nodes[i]);
	}

	// append additional routing information about this node
	DYMO_AddressBlock additional_node;
	additional_node.setDist(0);
	additional_node.setAddress(myAddr);
	if (RESPONSIBLE_ADDRESSES_PREFIX != -1) additional_node.setPrefix(RESPONSIBLE_ADDRESSES_PREFIX);
	incSeqNum();
	additional_node.setSeqNum(ownSeqNum);
	additional_nodes_to_relay.push_back(additional_node);

	routingMsg->setAdditionalNodes(additional_nodes_to_relay);
	routingMsg->setMsgHdrHopLimit(routingMsg->getMsgHdrHopLimit() - 1);

	// check hop limit
	if (routingMsg->getMsgHdrHopLimit() < 1) {
		ev << "received message has reached hop limit -> delete message" << endl;
		delete routingMsg;
		return;
	}

	// do not transmit DYMO messages when we lost our sequence number
	if (ownSeqNumLossTimeout->isRunning()) {
		ev << "node has lost sequence number -> not transmitting anything" << endl;
		delete routingMsg;
		return;
	}

	// do rate limiting
	if ((dynamic_cast<DYMO_RREQ*>(routingMsg)) && (!rateLimiterRREQ->consumeTokens(1, simTime()))) {
		ev << "RREQ send rate exceeded maximum -> not transmitting RREQ" << endl;
		delete routingMsg;
		return;
	}

	/* transmit message -- RREP via unicast, RREQ via DYMOcast */
	sendDown(routingMsg, dynamic_cast<DYMO_RREP*>(routingMsg) ? (entry->routeNextHopAddress).getInt() : LL_MANET_ROUTERS.getInt());

	/* keep statistics */
	if (dynamic_cast<DYMO_RREP*>(routingMsg)) {
		statsRREPFwd++;
	} else {
		statsRREQFwd++;
	}
}

void DYMOnet__DYMO::handleLowerRERR(DYMO_RERR *my_rerr) {
	/** message is a RERR. **/
	statsDYMORcvd++;

	// get RERR's IP.SourceAddress
	//UDPControlInfo* controlInfo = check_and_cast<UDPControlInfo*>(my_rerr->getControlInfo());
	IPAddress sourceAddr = my_rerr->getSenderModuleId();

	// get RERR's SourceInterface
	InterfaceEntry* sourceInterface = getNextHopInterface(my_rerr);

	ev << "Received RERR from " << sourceAddr << endl;

	// iterate over all unreachableNode entries
	std::vector<DYMO_AddressBlock> unreachableNodes = my_rerr->getUnreachableNodes();
	std::vector<DYMO_AddressBlock> unreachableNodesToForward;
	for(unsigned int i = 0; i < unreachableNodes.size(); i++) {
		const DYMO_AddressBlock& unreachableNode = unreachableNodes[i];

		if (IPAddress(unreachableNode.getAddress()).isMulticast()) continue;

		// check whether this invalidates entries in our routing table
		std::vector<DYMO_RoutingEntry *> RouteVector = dymo_routingTable->getRoutingTable();
		for(unsigned int i = 0; i < RouteVector.size(); i++) {
			DYMO_RoutingEntry* entry = RouteVector[i];

			// skip if route has no associated Forwarding Route
			if (entry->routeBroken) continue;

			// skip if this route isn't to the unreachableNode Address mentioned in the RERR
			if (!entry->routeAddress.prefixMatches(unreachableNode.getAddress(), entry->routePrefix)) continue;

			// skip if route entry isn't via the RERR sender
			if (entry->routeNextHopAddress != sourceAddr) continue;
			if (entry->routeNextHopInterface != sourceInterface) continue;

			// skip if route entry is fresher
			if (!((entry->routeSeqNum == 0) || (!unreachableNode.hasSeqNum()) || (!seqNumIsFresher(entry->routeSeqNum, unreachableNode.getSeqNum())))) continue;

			ev << "RERR invalidates route to " << entry->routeAddress << " via " << entry->routeNextHopAddress << endl;

			// mark as broken and delete associated forwarding route
			entry->routeBroken = true;
			dymo_routingTable->maintainAssociatedRoutingTable();

			// start delete timer
			// TODO: not specified in draft, but seems to make sense
			entry->routeDelete.start(ROUTE_DELETE_TIMEOUT);

			// update unreachableNode.SeqNum
			// TODO: not specified in draft, but seems to make sense
			DYMO_AddressBlock unreachableNodeToForward;
			unreachableNodeToForward.setAddress(unreachableNode.getAddress());
			if (unreachableNode.hasSeqNum()) unreachableNodeToForward.setSeqNum(unreachableNode.getSeqNum());
			if (entry->routeSeqNum != 0) unreachableNodeToForward.setSeqNum(entry->routeSeqNum);

			// forward this unreachableNode entry
			unreachableNodesToForward.push_back(unreachableNodeToForward);
		}
	}

	// discard RERR if there are no entries to forward
	if (unreachableNodesToForward.size() <= 0) {
		statsRERRRcvd++;
		delete my_rerr;
		return;
	}

	// discard RERR if ownSeqNum was lost
	if (ownSeqNumLossTimeout->isRunning()) {
		statsRERRRcvd++;
		delete my_rerr;
		return;
	}

	// discard RERR if msgHdrHopLimit has reached 1
	if (my_rerr->getMsgHdrHopLimit() <= 1) {
		statsRERRRcvd++;
		delete my_rerr;
		return;
	}

	// forward RERR with unreachableNodesToForward
	my_rerr->setUnreachableNodes(unreachableNodesToForward);
	my_rerr->setMsgHdrHopLimit(my_rerr->getMsgHdrHopLimit() - 1);

	ev << "send down RERR" << endl;
	sendDown(my_rerr, LL_MANET_ROUTERS.getInt());

	statsRERRFwd++;
}

void DYMOnet__DYMO::handleLowerUERR(DYMO_UERR *my_uerr) {
	/** message is a UERR. **/
	statsDYMORcvd++;

	ev << "Received unsupported UERR message" << endl;
	// to be finished
	delete my_uerr;
}

void DYMOnet__DYMO::handleSelfMsg(cMessage* apMsg) {
	ev << "handle self message" << endl;
	if(dynamic_cast<DYMO_Timeout*>(apMsg) != NULL) {
		// Something timed out. Let's find out what.

		// Maybe it's a ownSeqNumLossTimeout
		if (ownSeqNumLossTimeout->isExpired() || ownSeqNumLossTimeoutMax->isExpired()) {
			ownSeqNumLossTimeout->cancel();
			ownSeqNumLossTimeoutMax->cancel();
			ownSeqNum = 1;
		}

		// Maybe it's a outstanding RREQ
		DYMO_OutstandingRREQ* outstandingRREQ = outstandingRREQList.getExpired();
		if (outstandingRREQ) {
			handleRREQTimeout(*outstandingRREQ);
		}

		// Maybe it's a DYMO_RoutingEntry
		for(int i = 0; i < dymo_routingTable->getNumRoutes(); i++) {
			DYMO_RoutingEntry *entry = dymo_routingTable->getRoute(i);
			if (entry->routeAgeMin.isExpired()) {
				break;
			}
			if (entry->routeAgeMax.isExpired()) {
				dymo_routingTable->deleteRoute(entry);
				break; // if other timeouts also expired, they will have gotten their own DYMO_Timeout scheduled, so it's ok to stop here
			}
			if (entry->routeNew.isExpired()) {
				if (!entry->routeUsed.isRunning()) {
					entry->routeDelete.start(ROUTE_DELETE_TIMEOUT);
				}
				break;
			}
			if (entry->routeUsed.isExpired()) {
				if (!entry->routeNew.isRunning()) {
					entry->routeDelete.start(ROUTE_DELETE_TIMEOUT);
				}
				break;
			}
			if (entry->routeDelete.isExpired()) {
				dymo_routingTable->deleteRoute(entry);
				break;
			}
		}

	}
	else error("unknown message type");

}

DYMO_Packet * DYMOnet__DYMO::encapsMsg(cPacket * apMsg) {

	AppPkt * control_info = check_and_cast<AppPkt *>(apMsg->removeControlInfo());
	DYMO_Packet * dymoPkt = new DYMO_Packet(apMsg->getName());

	cModule* destinationModule = getParentModule()->getParentModule()->getModuleByRelativePath(control_info->getDestName());
	if(!destinationModule) error("destination module not found");

	/** get the IP-Address of the destination node **/
	unsigned int destAddr = destinationModule->getId();

	dymoPkt->setDestAddress(destAddr);
	dymoPkt->setSrcAddress(myAddr);
	dymoPkt->setCreationTime(simTime());

	dymoPkt->encapsulate(apMsg);
	delete control_info;

	return dymoPkt;
}

void DYMOnet__DYMO::sendDown(cPacket* apMsg, int destAddr) {
	// WARNING: This method uses lots of hacks and/or guesswork to determine output gate and message recipient!
	// More specifically, Hosts are assumed to only have interfaces "wlan", "eth" or both. In the latter case, lowergateOut[1] is expected to be connected to the "eth" interface.

	// all messages sent to a lower layer are delayed by 0..MAXJITTER seconds (draft-ietf-manet-jitter-01)
	simtime_t jitter = dblrand() * MAXJITTER;

	// set byte size of message
	const DYMO_RM* re = dynamic_cast<const DYMO_RM*>(apMsg);
	const DYMO_RERR* rerr = dynamic_cast<const DYMO_RERR*>(apMsg);
	const DYMO_Packet* pkt = dynamic_cast<const DYMO_Packet*>(apMsg);
	if (re) {
		apMsg->setByteLength(DYMO_RM_HEADER_LENGTH + ((1 + re->getAdditionalNodes().size()) * DYMO_RBLOCK_LENGTH));
	}
	else if (rerr) {
		apMsg->setByteLength(DYMO_RERR_HEADER_LENGTH + (rerr->getUnreachableNodes().size() * DYMO_UBLOCK_LENGTH));
	}
	else if (pkt) {
		// A DYMO_Packet is considered to be sent unencapsulated, so we trust the original byte size to be set correctly
	}
	else {
		error("tried to send unsupported message type");
	}

	if(destAddr == (int)LL_MANET_ROUTERS.getInt()) {
		MACAddress mac_dest = MACAddress::BROADCAST_ADDRESS;
		apMsg->removeControlInfo();
		Ieee802Ctrl *control_info = new Ieee802Ctrl();
		control_info->setDest(mac_dest);
		apMsg->setControlInfo(control_info);

		if (mLowergateOut[0] != -1) sendDelayed(apMsg, jitter, mLowergateOut[0]);
		if (mLowergateOut[1] != -1) {
			cPacket* apMsg2 = (cPacket*)apMsg->dup();
			apMsg2->removeControlInfo();
			Ieee802Ctrl *control_info2 = new Ieee802Ctrl();
			control_info2->setDest(mac_dest);
			apMsg2->setControlInfo(control_info2);
			sendDelayed(apMsg2, jitter, mLowergateOut[1]);
		}

		return;
	}

	cModule* srcHost = getParentModule();
	if (!srcHost) error("source getModule(i.e. our parent) for new message not found");
	IInterfaceTable* srcIftable = check_and_cast<IInterfaceTable*>(srcHost->getSubmodule("interfaceTable"));
	if (!srcIftable) error("source module for new message has no interface table \"interfaceTable\"");
	cModule* dstHost = simulation.getModule(destAddr);
	if (!dstHost) error("destination module for new message not found");
	IInterfaceTable* dstIftable = check_and_cast<IInterfaceTable*>(dstHost->getSubmodule("interfaceTable"));
	if (!dstIftable) error("destination module for new message has no interface table \"interfaceTable\"");

	InterfaceEntry* srcWlanIf = srcIftable->getInterfaceByName("wlan");
	InterfaceEntry* srcEthIf = srcIftable->getInterfaceByName("eth");
	InterfaceEntry* dstWlanIf = dstIftable->getInterfaceByName("wlan");
	InterfaceEntry* dstEthIf = dstIftable->getInterfaceByName("eth");

	int gateIndex;
	if (srcWlanIf && !srcEthIf) {
		gateIndex = 0;
	}
	else if (!srcWlanIf && srcEthIf) {
		gateIndex = 0;
	}
	else if (srcWlanIf && srcEthIf) {
		if (dstWlanIf && !dstEthIf) {
			gateIndex = 0;
		}
		else if (!dstWlanIf && dstEthIf) {
			gateIndex = 1;
		}
		else if (dstWlanIf && dstEthIf) {
			gateIndex = 1;
		}
		else error("destination host has neither an interface called \"wlan\" nor an interface called \"eth\" registered in its interfaceTable //0");

	}
	else error("source host has neither an interface called \"wlan\" nor an interface called \"eth\" registered in its interfaceTable //1");

	MACAddress mac_dst;
	if (dstWlanIf && !dstEthIf) {
		mac_dst = dstWlanIf->getMacAddress();
	}
	else if (!dstWlanIf && dstEthIf) {
		mac_dst = dstEthIf->getMacAddress();
	}
	else if (dstWlanIf && dstEthIf) {
		if (srcWlanIf && !srcEthIf) {
			mac_dst = dstWlanIf->getMacAddress();
		}
		else if (!srcWlanIf && srcEthIf) {
			mac_dst = dstEthIf->getMacAddress();
		}
		else if (srcWlanIf && srcEthIf) {
			mac_dst = dstEthIf->getMacAddress();
		}
		else error("source host has neither an interface called \"wlan\" nor an interface called \"eth\" registered in its interfaceTable //2");
	}
	else error("destination host has neither an interface called \"wlan\" nor an interface called \"eth\" registered in its interfaceTable //3");

	apMsg->removeControlInfo();
	Ieee802Ctrl* control_info = new Ieee802Ctrl();
	control_info->setDest(mac_dst);
	apMsg->setControlInfo(control_info);

	sendDelayed(apMsg, jitter, mLowergateOut[gateIndex]);
}

void DYMOnet__DYMO::sendUp(cPacket* apMsg) {
	ev << "sending up " << apMsg << "\n";
	send(apMsg, mUppergateOut);
}

DYMO_Packet* DYMOnet__DYMO::getQueuedMsg(unsigned int destAddr){
	cQueue::Iterator queueIterator(*queuedElements);
	QueueElement * qElement;

	while(!queueIterator.end()){
		qElement = dynamic_cast<QueueElement*>(queueIterator());
		if ((qElement) && (qElement->getDestAddr() == destAddr)) {
			return dynamic_cast<DYMO_Packet*>(qElement->getObject());
		}
		queueIterator++;
	}
	return NULL;
}

void DYMOnet__DYMO::removeQueuedMsg(unsigned int destAddr){
	cQueue::Iterator queueIterator(*queuedElements);
	QueueElement * qElement;

	while((!queueIterator.end()) && (queuedElements->length() > 0)){
		qElement = dynamic_cast<QueueElement*>(queueIterator());
		if ((qElement) && (qElement->getDestAddr() == destAddr)) {
			delete queuedElements->remove(queueIterator());
			return;
		}
		queueIterator++;
	}
}

void DYMOnet__DYMO::sendRREQ(unsigned int destAddr, int msgHdrHopLimit, unsigned int targetSeqNum, unsigned int targetDist) {
	/** generate a new RREQ with the given pararmeter **/
	ev << "send a RREQ to discover route to destination node " << destAddr << endl;

	DYMO_RM *my_rreq = new DYMO_RREQ("RREQ");
	my_rreq->setMsgHdrHopLimit(msgHdrHopLimit);
	my_rreq->getTargetNode().setAddress(destAddr);
	if (targetSeqNum != 0) my_rreq->getTargetNode().setSeqNum(targetSeqNum);
	if (targetDist != 0) my_rreq->getTargetNode().setDist(targetDist);

	my_rreq->getOrigNode().setDist(0);
	my_rreq->getOrigNode().setAddress(myAddr);
	if (RESPONSIBLE_ADDRESSES_PREFIX != -1) my_rreq->getOrigNode().setPrefix(RESPONSIBLE_ADDRESSES_PREFIX);
	incSeqNum();
	my_rreq->getOrigNode().setSeqNum(ownSeqNum);

	// do not transmit DYMO messages when we lost our sequence number
	if (ownSeqNumLossTimeout->isRunning()) {
		ev << "node has lost sequence number -> not transmitting RREQ" << endl;
		delete my_rreq;
		return;
	}

	// do rate limiting
	if (!rateLimiterRREQ->consumeTokens(1, simTime())) {
		ev << "RREQ send rate exceeded maximum -> not transmitting RREQ" << endl;
		delete my_rreq;
		return;
	}

	sendDown(my_rreq, LL_MANET_ROUTERS.getInt());
	statsRREQSent++;
}

void DYMOnet__DYMO::sendReply(unsigned int destAddr, unsigned int tSeqNum) {
	/** create a new RREP and send it to given destination **/
	ev << "send a reply to destination node " << destAddr << endl;

	DYMO_RM * rrep = new DYMO_RREP("RREP");
	DYMO_RoutingEntry *entry = dymo_routingTable->getForAddress(destAddr);
	if (!entry) error("Tried sending RREP via a route that just vanished");

	rrep->setMsgHdrHopLimit(MAX_HOPLIMIT);
	rrep->getTargetNode().setAddress(destAddr);
	rrep->getTargetNode().setSeqNum(entry->routeSeqNum);
	rrep->getTargetNode().setDist(entry->routeDist);

	// check if ownSeqNum should be incremented
	if ((tSeqNum == 0) || (seqNumIsFresher(ownSeqNum, tSeqNum))) incSeqNum();

	rrep->getOrigNode().setAddress(myAddr);
	if (RESPONSIBLE_ADDRESSES_PREFIX != -1) rrep->getOrigNode().setPrefix(RESPONSIBLE_ADDRESSES_PREFIX);
	rrep->getOrigNode().setSeqNum(ownSeqNum);
	rrep->getOrigNode().setDist(0);

	// do not transmit DYMO messages when we lost our sequence number
	if (ownSeqNumLossTimeout->isRunning()) {
		ev << "node has lost sequence number -> not transmitting anything" << endl;
		return;
	}

	sendDown(rrep, (entry->routeNextHopAddress).getInt());

	statsRREPSent++;
}

void DYMOnet__DYMO::sendReplyAsIntermediateRouter(const DYMO_AddressBlock& origNode, const DYMO_AddressBlock& targetNode, const DYMO_RoutingEntry* routeToTargetNode) {
	/** create a new RREP and send it to given destination **/
	ev << "sending a reply to OrigNode " << origNode.getAddress() << endl;

	DYMO_RoutingEntry* routeToOrigNode = dymo_routingTable->getForAddress(origNode.getAddress());
	if (!routeToOrigNode) error("no route to OrigNode found");

	// increment ownSeqNum.
	// TODO: The draft is unclear about when to increment ownSeqNum for intermediate DYMO router RREP creation
	incSeqNum();

	// create rrepToOrigNode
	DYMO_RREP* rrepToOrigNode = new DYMO_RREP("RREP");
	rrepToOrigNode->setMsgHdrHopLimit(MAX_HOPLIMIT);
	rrepToOrigNode->getTargetNode().setAddress(origNode.getAddress());
	rrepToOrigNode->getTargetNode().setSeqNum(origNode.getSeqNum());
	if (origNode.hasDist()) rrepToOrigNode->getTargetNode().setDist(origNode.getDist() + 1);

	rrepToOrigNode->getOrigNode().setAddress(myAddr);
	if (RESPONSIBLE_ADDRESSES_PREFIX != -1) rrepToOrigNode->getOrigNode().setPrefix(RESPONSIBLE_ADDRESSES_PREFIX);
	rrepToOrigNode->getOrigNode().setSeqNum(ownSeqNum);
	rrepToOrigNode->getOrigNode().setDist(0);

	DYMO_AddressBlock additionalNode;
	additionalNode.setAddress(routeToTargetNode->routeAddress.getInt());
	if (routeToTargetNode->routeSeqNum != 0) additionalNode.setSeqNum(routeToTargetNode->routeSeqNum);
	if (routeToTargetNode->routeDist != 0) additionalNode.setDist(routeToTargetNode->routeDist);
	rrepToOrigNode->getAdditionalNodes().push_back(additionalNode);

	// create rrepToTargetNode
	DYMO_RREP* rrepToTargetNode = new DYMO_RREP("RREP");
	rrepToTargetNode->setMsgHdrHopLimit(MAX_HOPLIMIT);
	rrepToTargetNode->getTargetNode().setAddress(targetNode.getAddress());
	if (targetNode.hasSeqNum()) rrepToTargetNode->getTargetNode().setSeqNum(targetNode.getSeqNum());
	if (targetNode.hasDist()) rrepToTargetNode->getTargetNode().setDist(targetNode.getDist());

	rrepToTargetNode->getOrigNode().setAddress(myAddr);
	if (RESPONSIBLE_ADDRESSES_PREFIX != -1) rrepToTargetNode->getOrigNode().setPrefix(RESPONSIBLE_ADDRESSES_PREFIX);
	rrepToTargetNode->getOrigNode().setSeqNum(ownSeqNum);
	rrepToTargetNode->getOrigNode().setDist(0);

	DYMO_AddressBlock additionalNode2;
	additionalNode2.setAddress(origNode.getAddress());
	additionalNode2.setSeqNum(origNode.getSeqNum());
	if (origNode.hasDist()) additionalNode2.setDist(origNode.getDist() + 1);
	rrepToTargetNode->getAdditionalNodes().push_back(additionalNode2);

	// do not transmit DYMO messages when we lost our sequence number
	if (ownSeqNumLossTimeout->isRunning()) {
		ev << "node has lost sequence number -> not transmitting anything" << endl;
		return;
	}

	sendDown(rrepToOrigNode, (routeToOrigNode->routeNextHopAddress).getInt());
	sendDown(rrepToTargetNode, (routeToTargetNode->routeNextHopAddress).getInt());

	statsRREPSent++;
}

void DYMOnet__DYMO::sendRERR(unsigned int targetAddr, unsigned int targetSeqNum) {
	ev << "generating an RERR" << endl;
	DYMO_RERR *rerr = new DYMO_RERR("RERR");
	std::vector<DYMO_AddressBlock> unode_vec;

	// add target node as first unreachableNode
	DYMO_AddressBlock unode;
	unode.setAddress(targetAddr);
	if (targetSeqNum != 0) unode.setSeqNum(targetSeqNum);
	unode_vec.push_back(unode);

	// set hop limit
	rerr->setMsgHdrHopLimit(MAX_HOPLIMIT);

	// add additional unreachableNode entries for all route entries that use the same routeNextHopAddress and routeNextHopInterface
	DYMO_RoutingEntry* brokenEntry = dymo_routingTable->getForAddress(targetAddr);
	if (brokenEntry) {
		// sanity check
		if (!brokenEntry->routeBroken) throw std::runtime_error("sendRERR called for targetAddr that has a perfectly fine routing table entry");

		// add route entries with same routeNextHopAddress as broken route
		std::vector<DYMO_RoutingEntry *> RouteVector = dymo_routingTable->getRoutingTable();
		for(unsigned int i = 0; i < RouteVector.size(); i++) {
			DYMO_RoutingEntry* entry = RouteVector[i];
			if ((entry->routeNextHopAddress != brokenEntry->routeNextHopAddress) || (entry->routeNextHopInterface != brokenEntry->routeNextHopInterface)) continue;

			ev << "Including in RERR route to " << entry->routeAddress << " via " << entry->routeNextHopAddress << endl;

			DYMO_AddressBlock unode;
			unode.setAddress(entry->routeAddress.getInt());
			if (entry->routeSeqNum != 0) unode.setSeqNum(entry->routeSeqNum);
			unode_vec.push_back(unode);
		}
	}

	// wrap up and send
	rerr->setUnreachableNodes(unode_vec);
	sendDown(rerr, LL_MANET_ROUTERS.getInt());

	// keep statistics
	statsRERRSent++;
}

void DYMOnet__DYMO::incSeqNum() {
	if(ownSeqNum == 0xffff) {
		ownSeqNum = 0x0100;
	} else {
		ownSeqNum++;
	}
}

bool DYMOnet__DYMO::seqNumIsFresher(unsigned int seqNumInQuestion, unsigned int referenceSeqNum) {
	return ((int16_t)referenceSeqNum - (int16_t)seqNumInQuestion < 0);
}

simtime_t DYMOnet__DYMO::computeBackoff(simtime_t backoff_var) {
	return backoff_var * 2;
}

void DYMOnet__DYMO::updateRouteLifetimes(unsigned int targetAddr) {
	DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(targetAddr);
	if (!entry) return;

	// TODO: not specified in draft, but seems to make sense
	if (entry->routeBroken) return;

	entry->routeUsed.start(ROUTE_USED_TIMEOUT);
	entry->routeDelete.cancel();

	dymo_routingTable->maintainAssociatedRoutingTable();
	ev << "lifetimes of route to destination node " << targetAddr << " are up to date "  << endl;

	checkAndSendQueuedPkts(entry->routeAddress.getInt(), entry->routePrefix, (entry->routeNextHopAddress).getInt());
}

bool DYMOnet__DYMO::isRBlockBetter(DYMO_RoutingEntry * entry, DYMO_AddressBlock ab, bool isRREQ) {
	//TODO: check handling of unknown SeqNum values

	// stale?
	if (seqNumIsFresher(entry->routeSeqNum, ab.getSeqNum())) return false;

	// loop-possible or inferior?
	if (ab.getSeqNum() == (int)entry->routeSeqNum) {

		int nodeDist = ab.hasDist() ? (ab.getDist() + 1) : 0; // incremented by one, because draft -10 says to first increment, then compare
		int routeDist = entry->routeDist;

		// loop-possible?
		if (nodeDist == 0) return false;
		if (routeDist == 0) return false;
		if (nodeDist > routeDist + 1) return false;

		// inferior?
		if (nodeDist > routeDist) return false;
		if ((nodeDist == routeDist) && (!entry->routeBroken) && (isRREQ)) return false;

	}

	// superior
	return true;
}

void DYMOnet__DYMO::handleRREQTimeout(DYMO_OutstandingRREQ& outstandingRREQ) {
	ev << "Handling RREQ Timeouts for RREQ to " << outstandingRREQ.destAddr << endl;

	if(outstandingRREQ.tries < RREQ_TRIES) {
		DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(outstandingRREQ.destAddr);
		if(entry && (!entry->routeBroken)) {
			/** an entry was found in the routing table -> get control data from the table, encapsulate message **/
			ev << "RREQ timed out and we DO have a route" << endl;

			checkAndSendQueuedPkts(entry->routeAddress.getInt(), entry->routePrefix, entry->routeNextHopAddress.getInt());

			return;
		}
		else {
			ev << "RREQ timed out and we do not have a route yet" << endl;
			/** number of tries is less than RREQ_TRIES -> backoff and send the rreq **/
			outstandingRREQ.tries = outstandingRREQ.tries + 1;
			outstandingRREQ.wait_time->start(computeBackoff(outstandingRREQ.wait_time->getInterval()));

			/* update seqNum */
			incSeqNum();

			unsigned int targetSeqNum = 0;
			// if a targetSeqNum is known, include it in all but the last RREQ attempt
			if (entry && (outstandingRREQ.tries < RREQ_TRIES)) targetSeqNum = entry->routeSeqNum;

			// expanding ring search
			int msgHdrHopLimit = MIN_HOPLIMIT + (MAX_HOPLIMIT - MIN_HOPLIMIT) * (outstandingRREQ.tries - 1) / (RREQ_TRIES - 1);

			sendRREQ(outstandingRREQ.destAddr, msgHdrHopLimit, targetSeqNum, (entry?(entry->routeDist):0));

			return;
		}
	} else {
		/** RREQ_TRIES is reached **/

		/** send an unreachable ICMP message to higher layer if original message sent from that layer **/
		char str[30];
		sprintf(str,"node with address %d", outstandingRREQ.destAddr);
		ICMPMessage * icmpMsg = new ICMPMessage(str);
		icmpMsg->setType(ICMP_DESTINATION_UNREACHABLE);

		sendUp(icmpMsg);

		cPacket *msg;
		while((msg=getQueuedMsg(outstandingRREQ.destAddr))) {
			DYMO_Packet *pkt=dynamic_cast<DYMO_Packet *>(msg);
			if(!pkt) continue;
			removeQueuedMsg(outstandingRREQ.destAddr);
			delete pkt;
		}

		// clean up outstandingRREQList
		outstandingRREQList.del(&outstandingRREQ);

		return;
	}

	return;
}

bool DYMOnet__DYMO::updateRoutesFromAddressBlock(const DYMO_AddressBlock& ab, bool isRREQ, uint32_t nextHopAddress, InterfaceEntry* nextHopInterface) {
	DYMO_RoutingEntry* entry = dymo_routingTable->getForAddress(IPAddress(ab.getAddress()));
	if (entry && !isRBlockBetter(entry, ab, isRREQ)) return false;

	if (!entry) {
		ev << "adding routing entry for " << IPAddress(ab.getAddress()) << endl;
		entry = new DYMO_RoutingEntry(this);
		dymo_routingTable->addRoute(entry);
	} else {
		ev << "updating routing entry for " << IPAddress(ab.getAddress()) << endl;
	}

	entry->routeAddress = ab.getAddress();
	entry->routeSeqNum = ab.getSeqNum();
	entry->routeDist = ab.hasDist() ? (ab.getDist() + 1) : 0;  // incremented by one, because draft -10 says to first increment, then compare
	entry->routeNextHopAddress = nextHopAddress;
	entry->routeNextHopInterface = nextHopInterface;
	entry->routePrefix = ab.hasPrefix() ? ab.getPrefix() : 32;
	entry->routeBroken = false;
	entry->routeAgeMin.start(ROUTE_AGE_MIN_TIMEOUT);
	entry->routeAgeMax.start(ROUTE_AGE_MAX_TIMEOUT);
	entry->routeNew.start(ROUTE_NEW_TIMEOUT);
	entry->routeUsed.cancel();
	entry->routeDelete.cancel();

	dymo_routingTable->maintainAssociatedRoutingTable();

	checkAndSendQueuedPkts(entry->routeAddress.getInt(), entry->routePrefix, nextHopAddress);

	return true;
}

DYMO_RM* DYMOnet__DYMO::updateRoutes(DYMO_RM * pkt) {
	ev << "starting update routes from routing blocks in the received message" << endl;
	std::vector<DYMO_AddressBlock> additional_nodes = pkt->getAdditionalNodes();
	std::vector<DYMO_AddressBlock> new_additional_nodes;

	bool isRREQ = (dynamic_cast<DYMO_RREQ*>(pkt) != 0);
	uint32_t nextHopAddress = getNextHopAddress(pkt);
	InterfaceEntry* nextHopInterface = getNextHopInterface(pkt);

	if(pkt->getOrigNode().getAddress()==myAddr) return NULL;
	bool origNodeEntryWasSuperior = updateRoutesFromAddressBlock(pkt->getOrigNode(), isRREQ, nextHopAddress, nextHopInterface);

	for(unsigned int i = 0; i < additional_nodes.size(); i++) {

		// TODO: not specified in draft, but seems to make sense
		if(additional_nodes[i].getAddress()==myAddr) return NULL;

		if (updateRoutesFromAddressBlock(additional_nodes[i], isRREQ, nextHopAddress, nextHopInterface)) {
			/** read routing block is valid -> save block to the routing message **/
			new_additional_nodes.push_back(additional_nodes[i]);
		} else {
			ev << "AdditionalNode AddressBlock has no valid information  -> dropping block from routing message" << endl;
		}
	}

	if (!origNodeEntryWasSuperior) {
		ev << "OrigNode AddressBlock had no valid information -> deleting received routing message" << endl;
		return NULL;
	}

	pkt->setAdditionalNodes(new_additional_nodes);

	return pkt;
}

void DYMOnet__DYMO::checkAndSendQueuedPkts(unsigned int destinationAddress, int prefix, unsigned int nextHopAddress)
{
	cPacket *msg;
	while((msg=getQueuedMsg(destinationAddress)))
	{
		DYMO_Packet *pkt=dynamic_cast<DYMO_Packet *>(msg);
		if(!pkt) continue;
		simtime_t dd = simTime() - pkt->getCreationTime();
		discoveryDelayVec.record(dd);

		discoveryLatency += dd;
		disSamples++;
		removeQueuedMsg(destinationAddress);
		statsTrafficSent++;
		pkt->setNextHopAddress(nextHopAddress);
		sendDown(pkt, nextHopAddress);
	}

	// clean up outstandingRREQList: remove those with matching destAddr
	DYMO_OutstandingRREQ* o = outstandingRREQList.getByDestAddr(destinationAddress, prefix);
	if (o) outstandingRREQList.del(o);
}

void DYMOnet__DYMO::setMyAddr(unsigned int myAddr) {
	// Check if this node has already participated in DYMO
	if (statsRREQSent || statsRREPSent || statsRERRSent) {
		// TODO: Send RERRs, cold-start DYMO, lose sequence number instead?
		ev << "Ignoring IP Address change request. This node has already participated in DYMO." << endl;
		return;
	}

	ev << "Now assuming this node is reachable at address " << myAddr << " (was " << this->myAddr << ")" << endl;
	this->myAddr = myAddr;

}

DYMO_RoutingTable* DYMOnet__DYMO::getDYMORoutingTable() {
	return dymo_routingTable;
}

cModule* DYMOnet__DYMO::getRouterByAddress(IPAddress address) {
	return dynamic_cast<cModule*>(simulation.getModule(address.getInt() - AUTOASSIGN_ADDRESS_BASE.getInt()));
}

