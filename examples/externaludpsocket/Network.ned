package inet.examples.externaludpsocket;

import inet.world.ExternalUdpSocket;
import inet.base.NotificationBoard;

network Network
{
    submodules:
        notificationBoard: NotificationBoard {
        }
        externalUdpSocket: ExternalUdpSocket {
        }
    connections:
}
