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

cplusplus {{
#include "networklayer/dymo/DYMO_PacketBBMessage_m.h"
#include "networklayer/dymo/DYMO_AddressBlock.h"

#include <vector>
#include <sys/types.h>

using namespace std;

typedef vector<DYMOnet::DYMO_AddressBlock> VectorOfDYMOAddressBlocks;

#include "networklayer/dymo/InspectorHelpers.h"

class DYMOnet_RM;

namespace DYMOnet {
  typedef DYMOnet_RM DYMO_RM;
}


}}


class noncobject VectorOfDYMOAddressBlocks;

class DYMOnet_PacketBBMessage;

struct DYMOnet__DYMO_AddressBlock;


message DYMOnet_RM extends DYMOnet_PacketBBMessage
{
    DYMOnet__DYMO_AddressBlock targetNode;
    DYMOnet__DYMO_AddressBlock origNode;
    VectorOfDYMOAddressBlocks additionalNodes;
}
