//
// Copyright (C) 2009 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef WORLD_EXTERNALUDPSOCKET_H
#define WORLD_EXTERNALUDPSOCKET_H

#include <string>
#include <ctype.h>

#include <omnetpp.h>
#include "INETDefs.h"
#include "NotificationBoard.h"


/**
 * ExternalUdpSocket handles events from a UDP Socket
 *
 * @author Christoph Sommer
 */
class INET_API ExternalUdpSocket : public cSimpleModule {
	public:
		struct Event : cPolymorphic {
			std::string packet;

			virtual std::string info() const {
				std::string r;

				r += "\"";
				for (size_t i = 0; i < std::min(packet.length(), static_cast<size_t>(16)); i++) {
					char c = packet[i];
					if (!isprint(c)) c = '.';
					r += c;
				}
				r += "\"";
				
				return r; 
			}
		};

	protected:
		virtual int numInitStages() const {return 5;}
		virtual void initialize(int stage);
		virtual void finish();
		virtual void handleMessage(cMessage* msg);

	protected:
		simtime_t updateInterval;

		cMessage* pollSocket;
		int sock_fd;
		NotificationBoard *nb;
};

#endif
