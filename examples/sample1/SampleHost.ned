// This module defines the sample host SampleHost
// basically consisting of the 802.11 (wlan) mac and physical layers,
// a sample routing protocol and a sample application

// You can use it as a starting point for your own modules.

// Important: Please rename all occurrences of "sample" 
// to something meaningful in the context of your module!

// author: Isabel Dietrich, isabel.dietrich@informatik.uni-erlangen.de

package inet.examples.sample1;

import inet.networklayer.sample.sampleRoutingProtocol;
import inet.networklayer.common.InterfaceTable;
import inet.mobility.BasicMobility;
import inet.linklayer.ieee80211.Ieee80211NicAdhoc;
import inet.base.NotificationBoard;
import inet.applications.sample.sampleApp;


module SampleHost
{
    parameters:
        string mobilityType;
        volatile double numHosts;
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
        appl: sampleApp {
            parameters:
                @display("p=163,67;i=block/app");

        }
        net: sampleRoutingProtocol {
            parameters:
                @display("p=248,247;i=block/fork;q=queue");

        }
        wlan: Ieee80211NicAdhoc {
            parameters:
                @display("p=248,349;q=queue;i=block/ifcard");

        }
    connections allowunconnected:
        // connections to network outside
        radioIn --> wlan.radioIn;
        wlan.uppergateOut --> net.ifIn;
        wlan.uppergateIn <-- net.ifOut;

        net.uppergateOut --> appl.lowergateIn;
        net.uppergateIn <-- appl.lowergateOut;

}

