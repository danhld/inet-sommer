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


/**
 * ExternalJoystick handles events from a Joystick-like input device
 * attached to the computer running this module's simulation
 *
 * @author Christoph Sommer
 */
class INET_API ExternalJoystick : public cSimpleModule {
	public:

	protected:
		virtual int numInitStages() const {return 5;}
		virtual void initialize(int stage);
		virtual void finish();
		virtual void handleMessage(cMessage* msg);

		/**
		 * Called when one or more buttons were pressed since the last update.
		 * @param buttons bit buffer of buttons pressed
		 */
		virtual void onJoystickButtonsPressed(uint16_t buttons);

		/**
		 * Called when one or more buttons were released since the last update.
		 * @param buttons bit buffer of buttons released
		 */
		virtual void onJoystickButtonsReleased(uint16_t buttons);

	protected:
		simtime_t updateInterval;

		cMessage* pollJoystick;
		void* joystickState;

};

#endif
