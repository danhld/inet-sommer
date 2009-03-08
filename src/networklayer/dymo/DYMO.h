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

#ifndef NETWORKLAYER_DYMO_DYMO_H
#define NETWORKLAYER_DYMO_DYMO_H

#include <omnetpp.h>

#include "IPAddress.h"
#include "IInterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "IRoutingTable.h"
#include "RoutingTableAccess.h"
#include "Ieee802Ctrl_m.h"
#include "ICMPMessage.h"

#include "networklayer/dymo/DYMO_Packet_m.h" 
#include "networklayer/dymo/DYMO_RoutingTable.h"
#include "networklayer/dymo/DYMO_OutstandingRREQList.h"
#include "networklayer/dymo/DYMO_RM_m.h"
#include "networklayer/dymo/DYMO_RREQ_m.h"
#include "networklayer/dymo/DYMO_RREP_m.h"
#include "networklayer/dymo/DYMO_AddressBlock.h"
#include "networklayer/dymo/DYMO_RERR_m.h"
#include "networklayer/dymo/DYMO_UERR_m.h"
#include "networklayer/dymo/AppPkt_m.h"
#include "networklayer/dymo/DYMO_Timeout_m.h"
#include "networklayer/dymo/DYMO_TokenBucket.h"

using namespace DYMOnet;

namespace DYMOnet {

//===========================================================================================
// class QueueElement: defines a data type for all queued data messages
//===========================================================================================
class QueueElement : public cPacket
{
	public:
		QueueElement(){ seqNum = 0; destAddr = 0; srcAddr = 0;obj = NULL; };

		QueueElement(unsigned int seqNum, unsigned int destAddr, unsigned int srcAddr, DYMO_Packet* obj){
			this->seqNum = seqNum;
			this->destAddr = destAddr;
			this->srcAddr = srcAddr;
			this->obj = obj;
		};
		virtual ~QueueElement(){};
		unsigned int getSeqNum(){ return seqNum; };
		unsigned int getDestAddr(){ return destAddr; };
		unsigned int getSrcAddr(){ return srcAddr; };
		DYMO_Packet* getObject(){ return obj; };

	private:
		unsigned int seqNum;
		unsigned int destAddr;
		unsigned int srcAddr;
		DYMO_Packet* obj;

};

}

//===========================================================================================
// class DYMO: implements the network layer to route incoming messages
//===========================================================================================
class DYMOnet__DYMO : public cSimpleModule
{
	public:

		int numInitStages() const  {return 4;}
		void initialize(int);
		void finish();

		/** @brief Function called whenever a message arrives at the module */
		void handleMessage(cMessage * msg);

		/** @brief change the address we think we are reachable at. Does not(!) make sure we are actually reachable there. The change request will be ignored if this node has already participated in DYMO */
		void setMyAddr(unsigned int myAddr);

		/** @brief ... */
		DYMO_RoutingTable* getDYMORoutingTable();

		/** @brief guesses which router the given address belongs to, might return 0 */
		cModule* getRouterByAddress(IPAddress address);

	private:
		//===============================================================================
		// OPERATIONS
		//===============================================================================
		/** @brief Handle self messages such as timer... */
		virtual void handleSelfMsg(cMessage*);

		/** @brief Handle messages from upper layer */
		virtual void handleUpperMsg(cPacket*);

		/** @brief return destination interface for reply messages */
		InterfaceEntry* getNextHopInterface(DYMO_PacketBBMessage* pkt);

		/** @brief return destination address for reply messages */
		uint32_t getNextHopAddress(DYMO_RM *routingMsg);

		/** @brief Function handles lower messages: sends messages sent to the node to higher layer, forward messages to destination nodes... */
		virtual void handleLowerMsg(cPacket*);
		virtual void handleLowerRM(DYMO_RM*);
		virtual void handleLowerRMForMe(DYMO_RM *routingMsg);
		virtual void handleLowerRMForRelay(DYMO_RM *routingMsg);
		virtual void handleLowerRERR(DYMO_RERR*);
		virtual void handleLowerUERR(DYMO_UERR*);
		virtual void handleLowerPacket(DYMO_Packet*);

		/** @brief encapsulate packet */
		DYMO_Packet* encapsMsg(cPacket*);
		void sendDataPacket(DYMO_Packet *dymoPkt);

		/** @brief Function sends messages to lower layer (transport layer) */ 
		void sendDown(cPacket*, int);

		/** @brief Sends a message to the upper layer */
		void sendUp(cPacket*);

		/** @brief Returns the message queued with the given id */
		DYMO_Packet* getQueuedMsg(unsigned int destAddr);

		/** @brief removes the message queued with the given id */
		void removeQueuedMsg(unsigned int destAddr);

		/** @brief Increments the ownSeqNum */
		void incSeqNum();

		/** @brief Returns true if seqNumInQuestion is greater than referenceSeqNum, taking into account sequence number rollover */
		bool seqNumIsFresher(unsigned int seqNumInQuestion, unsigned int referenceSeqNum);

		/** @brief calculates a backoff value */
		simtime_t computeBackoff(simtime_t);

		/** @brief updates the lifetime (validTimeout) of a route */
		void updateRouteLifetimes(unsigned int destAddr);

		/** @brief generates and sends down a new rreq for given destination address */
		void sendRREQ(unsigned int destAddr, int msgHdrHopLimit, unsigned int targetSeqNum, unsigned int targetHopCnt);

		/** @brief Creates and sends a RREP to the given destination */
		void sendReply(unsigned int destAddr, unsigned int tSeqNum);

		/** @brief Creates and sends a RREP when this node is an intermediate router */
		void sendReplyAsIntermediateRouter(const DYMO_AddressBlock& origNode, const DYMO_AddressBlock& targetNode, const DYMO_RoutingEntry* routeToTarget);

		/** @brief generates and sends a RERR message */
		void sendRERR(unsigned int targetAddress, unsigned int targetSeqNum);

		/** @brief checks whether an AddressBlock's information is better than the current one or is stale/disregarded. Behaviour according to draft-ietf-manet-dymo-09, section 5.2.1 ("Judging New Routing Information's Usefulness") */ 
		bool isRBlockBetter(DYMO_RoutingEntry * entry, DYMO_AddressBlock rblock, bool isRREQ);

		/** @brief Function verifies the send tries for a given RREQ and send a new request if RREQ_TRIES is not reached, deletes conrol info from the queue if RREQ_TRIES is reached  and send an ICMP message to upper layer */
		void handleRREQTimeout(DYMO_OutstandingRREQ& outstandingRREQ);

		/** @brief updates routing entries from AddressBlock, returns whether AddressBlock should be kept */
		bool updateRoutesFromAddressBlock(const DYMO_AddressBlock& ab, bool isRREQ, uint32_t nextHopAddress, InterfaceEntry* nextHopInterface);

		/** @brief Function updates the routing entries from received AdditionalNodes. @see draft 4.2.1 */ 
		DYMO_RM* updateRoutes(DYMO_RM * pkt);

		/** @brief dequeue packets to destinationAddress */
		void checkAndSendQueuedPkts(unsigned int destinationAddress, int prefix, unsigned int nextHopAddress);

		//===============================================================================
		// MEMBER VARIABLES
		//===============================================================================
		/** @brief pointer to the routing table */
		DYMO_RoutingTable * dymo_routingTable;

		/** @brief runs after the node has lost its OwnSeqNum. When either ownSeqNumLossTimeout or ownSeqNumLossTimeoutMax expires, the resumes participation in DYMO */
		DYMO_Timer* ownSeqNumLossTimeout;

		/** @brief runs after the node has lost its OwnSeqNum. When either ownSeqNumLossTimeout or ownSeqNumLossTimeoutMax expires, the resumes participation in DYMO */
		DYMO_Timer* ownSeqNumLossTimeoutMax;

		/** @brief address of the node */
		unsigned int myAddr;

		/** @brief queue contains all the queued messages waiting for route discovery */
		cQueue * queuedElements;

		/** @brief sequence number of the node */
		unsigned int ownSeqNum;

		/** @brief vector contains the sent RREQ and waiting for a reply*/
		DYMO_OutstandingRREQList outstandingRREQList;

		/** @brief defines the routing block size */
		unsigned int rblockSize;

		//  cPacket * rreq;
		//  cPacket * rerr;

		int mHeaderLength;

		int statsTrafficSent; /**< number of injected Application-layer messages */
		int statsTrafficRcvd; /**< number of received Application-layer messages (for this node) */
		int statsTrafficFwd; /**< number of Application-layer messages sent on behalf of other nodes */

		int statsRREQSent; /**< number of generated DYMO RREQs */
		int statsRREPSent; /**< number of generated DYMO RREPs */
		int statsRERRSent; /**< number of generated DYMO RERRs */

		int statsRREQRcvd; /**< number of consumed DYMO RREQs */
		int statsRREPRcvd; /**< number of consumed DYMO RREPs */
		int statsRERRRcvd; /**< number of consumed DYMO RERRs */

		int statsRREQFwd; /**< number of forwarded (and processed) DYMO RREQs */
		int statsRREPFwd; /**< number of forwarded (and processed) DYMO RREPs */
		int statsRERRFwd; /**< number of forwarded (and processed) DYMO RERRs */

		int statsDYMORcvd; /**< number of observed DYMO messages */

		simtime_t discoveryLatency;
		int disSamples;
		simtime_t dataLatency;
		int dataSamples;

		cOutVector discoveryDelayVec;
		cOutVector dataDelayVec;
		cOutVector dataDelayPerHop;
		cOutVector dataSourceVec;
		cOutVector dataNumHops;
		//cOutVector dataDelay2Vec;
		//cOutVector dataLoadVec;
		//cOutVector controlLoadVec;
		
		DYMO_TokenBucket* rateLimiterRREQ;

		int RESPONSIBLE_ADDRESSES_PREFIX; /**< NED parameter: netmask of network this DYMO router is responsible for, -1 for self only */
		const char* DYMO_INTERFACES; /**< NED parameter: list of interfaces to run DYMO on, separated by a single space character */	
		IPAddress AUTOASSIGN_ADDRESS_BASE; /**< NED parameter: start of address range from which to automatically assign an address to the DYMO_INTERFACES */
		int ROUTE_AGE_MIN_TIMEOUT;
		int ROUTE_AGE_MAX_TIMEOUT;
		int ROUTE_NEW_TIMEOUT;
		int ROUTE_USED_TIMEOUT;
		int ROUTE_DELETE_TIMEOUT;
		int MIN_HOPLIMIT; /**< NED parameter: RREQs are first tried with this MsgHdr.HopLimit */
		int MAX_HOPLIMIT; /**< NED parameter: MsgHdr.HopLimit for last RREQ, as well as other DYMO control messages */
                double RREQ_RATE_LIMIT; /**< NED parameter: maximum average RREQs per second (token bucket) */
		int RREQ_BURST_LIMIT; /**< NED parameter: maximum RREQs in a burst (token bucket) */
		simtime_t RREQ_WAIT_TIME;
		unsigned int RREQ_TRIES;

		/** @brief gate id*/
		//@{
		int mUppergateIn;
		int mUppergateOut;
		int mLowergateIn[2];
		int mLowergateOut[2];
		//@}
};

#endif

