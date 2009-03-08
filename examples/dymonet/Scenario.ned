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

import inet.world.ChannelControl;

module Scenario
{
    parameters:
        int numHosts;
        double playgroundSizeX;
        double playgroundSizeY;
    submodules:
        host[numHosts]: SensorHost {
            parameters:
                numHosts = numHosts;
                @display("i=device/pocketpc_s;r=,,#707070");

        }
        sink: SensorHost {
            parameters:
                @display("i=device/pocketpc_s;r=,,#ff0000");

        }
        channelcontrol: ChannelControl {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                @display("p=-60,50;i=misc/sun");

        }
    connections allowunconnected:
}

network scenario extends Scenario
{
    parameters:
}

