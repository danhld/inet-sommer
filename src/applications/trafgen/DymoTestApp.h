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

/**
 * @short Application to test the DYMO routing protocol
 * @author Isabel Dietrich
*/

#ifndef DYMO_TEST_APP
#define DYMO_TEST_APP

#include "applications/trafgen/TrafGen.h"
#include "ICMPMessage.h"

class DymoTestApp : public TrafGen
{
public:

    // LIFECYCLE
    // this takes care of constructors and destructors

	virtual void initialize(int);
	virtual void finish();

protected:

  // OPERATIONS
	virtual void handleSelfMsg(cMessage*);
	virtual void handleLowerMsg(cPacket*);

	virtual void SendTraf(cPacket *msg, const char *dest);

private:

	int mLowergateIn;
	int mLowergateOut;

    int mCurrentTrafficPattern;

	double mNumTrafficMsgs;
	double mNumTrafficMsgRcvd;
	double mNumTrafficMsgNotDelivered;


};

#endif
