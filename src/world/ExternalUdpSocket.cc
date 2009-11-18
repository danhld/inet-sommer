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
#include <sys/socket.h>
#include <netinet/in.h>

#include "ExternalUdpSocket.h"

Define_Module(ExternalUdpSocket);

void ExternalUdpSocket::initialize(int stage) {
	if (stage == 4) {
		updateInterval = par("updateInterval");

		pollSocket = 0;
		sock_fd = 0;
		nb = 0;

		int port = 4444;

		// create socket
		sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

		// bind socket
		{
			struct sockaddr_in servaddr;
			bzero(&servaddr, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
			servaddr.sin_port=htons(port);

			bind(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		}

		nb = NotificationBoardAccess().get();

		pollSocket = new cMessage("pollSocket");
		scheduleAt(simTime(), pollSocket);
	}
}

void ExternalUdpSocket::finish() {
	if (sock_fd) close(sock_fd);
}

void ExternalUdpSocket::handleMessage(cMessage* msg) {
	if (msg != pollSocket) {
		error("Got invalid message");
		return;
	}

	// receive packet
	char mesg[1025];
	struct sockaddr_in cliaddr;
	socklen_t len = sizeof(cliaddr);
	int n = recvfrom(sock_fd, mesg, 1024, MSG_DONTWAIT, (struct sockaddr*)&cliaddr, &len);

	if (n != -1) {
		ExternalUdpSocket::Event currentEvent;
		currentEvent.packet = std::string(mesg, n);
		nb->fireChangeNotification(NF_EXTERNALUDPSOCKET_EVENT, &currentEvent);
	}

	scheduleAt(simTime() + updateInterval, pollSocket);
}

