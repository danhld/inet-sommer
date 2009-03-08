/*
 *  Copyright (C) 2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
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

#ifndef NETWORKLAYER_DYMO_DYMO_TOKENBUCKET_H
#define NETWORKLAYER_DYMO_DYMO_TOKENBUCKET_H

#include <omnetpp.h>

#include "IPAddress.h"
#include "InterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "RoutingTable.h"
#include "RoutingTableAccess.h"
#include "Ieee802Ctrl_m.h"
#include "ICMPMessage.h"
#include "IPDatagram.h"

#include "networklayer/dymo/DYMO_Packet_m.h" 
#include "networklayer/dymo/DYMO_RoutingTable.h"
#include "networklayer/dymo/DYMO_OutstandingRREQList.h"
#include "networklayer/dymo/DYMO_RM_m.h"
#include "networklayer/dymo/DYMO_RREQ_m.h"
#include "networklayer/dymo/DYMO_RREP_m.h"
#include "networklayer/dymo/DYMO_AddressBlock.h"
#include "networklayer/dymo/DYMO_RERR_m.h"
#include "networklayer/dymo/DYMO_UERR_m.h"
#include "networklayer/dymo/DYMO_Timeout_m.h"

namespace DYMOnet {

//===========================================================================================
// class DYMO_TokenBucket: Simple rate limiting mechanism
//===========================================================================================
class DYMO_TokenBucket {
	public:
		DYMO_TokenBucket(double tokensPerTick, double maxTokens, simtime_t currentTime);

		bool consumeTokens(double tokens, simtime_t currentTime);
		
	protected:
		double tokensPerTick;
		double maxTokens;

		double availableTokens;
		simtime_t lastUpdate;

};

}

#endif

