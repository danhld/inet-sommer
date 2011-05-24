// This module defines the sample network sampleNet,
// consisting of a number of SampleHosts
// and a ChannelControl module managing the connections

// You can use it as a starting point for your own modules.

// Important: Please rename all occurrences of "sample" 
// to something meaningful in the context of your module!

// author: Isabel Dietrich, isabel.dietrich@informatik.uni-erlangen.de

package inet.examples.sample1;

import inet.world.ChannelControl;

module SampleNet
{
    parameters:
        double numHosts;
        double playgroundSizeX;
        double playgroundSizeY;
    submodules:
        host[numHosts]: SampleHost {
            parameters:
                numHosts = numHosts;
                @display("i=device/pocketpc_s;r=,,#707070");

        }
        channelcontrol: ChannelControl {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                @display("p=-60,50;i=misc/sun");

        }
    connections allowunconnected:
}

network sampleNet extends SampleNet
{
    parameters:
}

