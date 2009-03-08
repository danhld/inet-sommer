//
//  Copyright (C) 2005 Mohamed Louizi
//  Copyright (C) 2006,2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

package inet.examples.dymonet;

import inet.base.NotificationBoard;
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.mf80211.Nic80211;
import inet.mobility.BasicMobility;
import inet.mobility.StaticGridMobility;
import inet.applications.trafgen.DymoTestApp;
import inet.networklayer.dymo.DYMOnet__DYMO;
import inet.linklayer.ieee80211.Ieee80211NicAdhoc;

module SensorHost
{
    parameters:
        string mobilityType;
        volatile int numHosts;
    gates:
        input radioIn;
    submodules:
        notificationBoard: NotificationBoard {
            parameters:
                @display("p=60,70;i=block/control");

        }
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=60,150;i=block/table");

        }
        mobility: <mobilityType> like BasicMobility {
            parameters:
                @display("p=149,307;i=block/cogwheel");

        }
        net: DYMOnet__DYMO {
            parameters:
                @display("p=248,247;i=block/fork;q=queue");

            gates:
                ifIn[1];
                ifOut[1];
        }
        appl: DymoTestApp {
            parameters:
                @display("p=163,67;i=block/app");

        }
        wlan: Ieee80211NicAdhoc {
            parameters:
                @display("p=248,349;q=queue;i=block/ifcard");

        }
    connections allowunconnected:
        // connections to network outside
        radioIn --> wlan.radioIn;
        wlan.uppergateOut --> net.ifIn[0];
        wlan.uppergateIn <-- net.ifOut[0];

        net.uppergate <--> appl.lowergate;
}

