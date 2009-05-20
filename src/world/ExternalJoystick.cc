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

#include <pthread.h>
#include <stdexcept>
#include <sstream>
#include "SDL.h"

#include "ExternalJoystick.h"

Define_Module(ExternalJoystick);


namespace {

	/**
	 * Reports Joystick button state changes between repeated calls to poll().
	 */
	class JoystickState {
		protected:
			/**
			 * Encapsulates SDL_Joystick*.
			 */
			struct Joystick {
				Joystick(int joyIndex) {
					if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) throw std::runtime_error("Could not init SDL Joystick subsystem");
					SDL_JoystickEventState(SDL_QUERY);
					joy = SDL_JoystickOpen(joyIndex);
					if (!joy) throw std::runtime_error("Could not open joystick");
				}
				~Joystick() {
					SDL_JoystickClose(joy);
					joy = 0;
				}
				SDL_Joystick* joy;
			};

			/**
			 * Encapsulates pthread_mutex_t.
			 */
			struct Mutex {
				Mutex() {
					pthread_mutex_init(&mutex, 0);
				}
				~Mutex() {
					pthread_mutex_destroy(&mutex);
				}
				void p() {
					pthread_mutex_lock(&mutex);
				}
				void v() {
					pthread_mutex_unlock(&mutex);
				}
				pthread_mutex_t mutex;
			};

		public:
			JoystickState(int joyIndex) : joyIndex(joyIndex), thread_kill(false), polledButtonState(0), thread_dead(false), joy(Joystick(joyIndex)), buttonState(0), buttonStateMux() {
				int r = pthread_create(&joythread, 0, JoystickState::joystickThread, (void *)this);
				if (r) throw std::runtime_error("Could not create Joystick thread");
			}

			~JoystickState() {
				thread_kill = true;
				pthread_join(joythread, 0);
			}

			/**
			 * Prepares button state changes since last call for retrieval via getPressed() and getReleased().
			 */
			void poll() {
				if (thread_dead) throw std::runtime_error("Joystick thread dead");

				buttonStateMux.p();
				polledButtonState = buttonState;
				buttonState = 0;
				buttonStateMux.v();
			}

			/**
			 * Returns bit mask of buttons pressed between last calls to poll().
			 */
			uint16_t getPressed() {
				return (0x0000FFFF & (polledButtonState));
			}

			/**
			 * Returns bit mask of buttons released between last calls to poll().
			 */
			uint16_t getReleased() {
				return (0x0000FFFF & (polledButtonState >> 16));
			}

		protected:
			/**
			 * Worker thread. Polls buttons and saves state.
			 */
			static void* joystickThread(void* joystickState_) {
				JoystickState& joystickState = *static_cast<JoystickState*>(joystickState_);

				uint32_t lastState = 0;
				while (!joystickState.thread_kill) {
					SDL_Delay(10);
					SDL_JoystickUpdate();

					uint32_t state = 0;
					for (int i= SDL_JoystickNumButtons(joystickState.joy.joy); i >= 0; i--) {
						bool isPressed = SDL_JoystickGetButton(joystickState.joy.joy, i);
						state = (state << 1) | isPressed;
					}
					uint32_t newBits = ((0x0000FFFF & (~state & lastState)) << 16) | ((0x0000FFFF & (state & ~lastState)));
					lastState = state;

					joystickState.buttonStateMux.p();
					joystickState.buttonState |= newBits;
					joystickState.buttonStateMux.v();
				}

				joystickState.thread_dead = true;
				return 0;
			}

		protected:
			int joyIndex; /**< SDL Joystick number to poll */
			pthread_t joythread;
			bool thread_kill; /**< true if thread should be killed ASAP */
			uint32_t polledButtonState; /**< holds buttonState after a call to poll() */
			bool thread_dead; /**< true if thread has terminated */
			Joystick joy;

			uint32_t buttonState; /**< two uint16_t of pressed and released buttons, respectively */
			Mutex buttonStateMux;
	};

}

void ExternalJoystick::initialize(int stage) {
	if (stage == 4) {
		updateInterval = par("updateInterval");
		pollJoystick = 0;
		joystickState = 0;

		joystickState = new JoystickState(0);

		pollJoystick = new cMessage("pollJoystick");
		scheduleAt(simTime(), pollJoystick);
	}
}

void ExternalJoystick::finish() {
	JoystickState* joystickState = static_cast<JoystickState*>(this->joystickState);
	delete joystickState;
	this->joystickState = 0;
}

void ExternalJoystick::handleMessage(cMessage* msg) {
	if (msg != pollJoystick) {
		error("Got invalid message");
		return;
	}

	JoystickState& joystickState = *static_cast<JoystickState*>(this->joystickState);

	joystickState.poll();
	uint16_t pressed = joystickState.getPressed();
	uint16_t released = joystickState.getReleased();

	if (pressed) onJoystickButtonsPressed(pressed);
	if (released) onJoystickButtonsReleased(released);

	scheduleAt(simTime() + updateInterval, pollJoystick);
}

void ExternalJoystick::onJoystickButtonsPressed(uint16_t buttons) {
	std::ostringstream ss;
	ss << "buttons " << (int)buttons << " pressed";
	bubble(ss.str().c_str());
}

void ExternalJoystick::onJoystickButtonsReleased(uint16_t buttons) {
	std::ostringstream ss;
	ss << "buttons " << (int)buttons << " released";
	bubble(ss.str().c_str());
}

