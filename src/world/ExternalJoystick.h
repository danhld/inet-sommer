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

#ifndef WORLD_EXTERNALJOYSTICK_H
#define WORLD_EXTERNALJOYSTICK_H

#include <omnetpp.h>
#include "INETDefs.h"
#include "NotificationBoard.h"


/**
 * ExternalJoystick handles events from a Joystick-like input device
 * attached to the computer running this module's simulation
 *
 * @author Christoph Sommer
 */
class INET_API ExternalJoystick : public cSimpleModule {
	public:
		struct Event : cPolymorphic {
			uint16_t buttonsPressed;
			uint16_t buttonsReleased;
		};

	protected:
		virtual int numInitStages() const {return 5;}
		virtual void initialize(int stage);
		virtual void finish();
		virtual void handleMessage(cMessage* msg);

	protected:
		simtime_t updateInterval;

		cMessage* pollJoystick;
		void* joystickState;

		NotificationBoard *nb;
};

#endif
