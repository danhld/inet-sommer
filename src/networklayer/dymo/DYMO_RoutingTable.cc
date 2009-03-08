/*
 *  Copyright (C) 2005 Mohamed Louizi
 *  Copyright (C) 2006,2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdexcept>
#include <sstream>
#include "networklayer/dymo/DYMO_RoutingTable.h"

namespace DYMOnet {

DYMO_RoutingTable::DYMO_RoutingTable(cModule* , const IPAddress& , const char* , const IPAddress& ) {

}

DYMO_RoutingTable::~DYMO_RoutingTable() {
	DYMO_RoutingEntry* entry;
	while ((entry = getRoute(0))) deleteRoute(entry);
}

const char* DYMO_RoutingTable::getFullName() const {
	return "DYMO_RoutingTable";
}

std::string DYMO_RoutingTable::info() const {
	std::ostringstream ss;

	ss << getNumRoutes() << " entries";

	int broken = 0;
	for (std::vector<DYMO_RoutingEntry *>::const_iterator iter = routeVector.begin(); iter < routeVector.end(); iter++) {
		DYMO_RoutingEntry* e = *iter;
		if (e->routeBroken) broken++;
	}
	ss << " (" << broken << " broken)";

	ss << " {" << std::endl;
	for (std::vector<DYMO_RoutingEntry *>::const_iterator iter = routeVector.begin(); iter < routeVector.end(); iter++) {
		DYMO_RoutingEntry* e = *iter;
		ss << "  " << *e << std::endl;
	}
	ss << "}";

	return ss.str();
}

std::string DYMO_RoutingTable::detailedInfo() const {
	return info();
}

//=================================================================================================
/*
 * Function returns the size of the table
 */ 
//=================================================================================================
int DYMO_RoutingTable::getNumRoutes() const {
  return (int)routeVector.size(); 
}

//=================================================================================================
/*
 * Function gets an routing entry at the given position 
 */ 
//=================================================================================================
DYMO_RoutingEntry* DYMO_RoutingTable::getRoute(int k){ 
  if(k < (int)routeVector.size())
    return routeVector[k];
  else
    return NULL;
}

//=================================================================================================
/*
 * 
 */ 
//=================================================================================================
void DYMO_RoutingTable::addRoute(DYMO_RoutingEntry *entry){
	routeVector.push_back(entry);
}

//=================================================================================================
/*
 */ 
//=================================================================================================
void DYMO_RoutingTable::deleteRoute(DYMO_RoutingEntry *entry){

	// update standard routingTable
	if (entry->routingEntry) {
		//routingTable->deleteRoute(entry->routingEntry);
		entry->routingEntry = 0;
	}

	// update DYMO routingTable
	std::vector<DYMO_RoutingEntry *>::iterator iter;
	for(iter = routeVector.begin(); iter < routeVector.end(); iter++){
		if(entry == *iter){
			routeVector.erase(iter);
			//updateDisplayString();
			delete entry;
			return;
		}
	}

	throw std::runtime_error("unknown routing entry requested to be deleted");
}

//=================================================================================================
/*
 */ 
//=================================================================================================
void DYMO_RoutingTable::maintainAssociatedRoutingTable() {
	std::vector<DYMO_RoutingEntry *>::iterator iter;
	for(iter = routeVector.begin(); iter < routeVector.end(); iter++){
		maintainAssociatedRoutingEntryFor(*iter);
	}
}

//=================================================================================================
/*
 */ 
//=================================================================================================
DYMO_RoutingEntry* DYMO_RoutingTable::getByAddress(IPAddress addr){

	std::vector<DYMO_RoutingEntry *>::iterator iter;
  
	for(iter = routeVector.begin(); iter < routeVector.end(); iter++){
		DYMO_RoutingEntry *entry = *iter;
    
		if(entry->routeAddress == addr){
			return entry;
		}
	}
  
	return 0;
}

//=================================================================================================
/*
 */ 
//=================================================================================================
DYMO_RoutingEntry* DYMO_RoutingTable::getForAddress(IPAddress addr) {
	std::vector<DYMO_RoutingEntry *>::iterator iter;

	int longestPrefix = 0; 
	DYMO_RoutingEntry* longestPrefixEntry = 0;
	for(iter = routeVector.begin(); iter < routeVector.end(); iter++) {
		DYMO_RoutingEntry *entry = *iter;

		// skip if we already have a more specific match
		if (!(entry->routePrefix > longestPrefix)) continue;

		// skip if address is not in routeAddress/routePrefix block
		if (!addr.prefixMatches(entry->routeAddress, entry->routePrefix)) continue;

		// we have a match
		longestPrefix = entry->routePrefix;
		longestPrefixEntry = entry;
	}

	return longestPrefixEntry;
}

//=================================================================================================
/*
 */ 
//=================================================================================================
DYMO_RoutingTable::RouteVector DYMO_RoutingTable::getRoutingTable(){
	return routeVector;
}

void DYMO_RoutingTable::maintainAssociatedRoutingEntryFor(DYMO_RoutingEntry* ){
}

std::ostream& operator<<(std::ostream& os, const DYMO_RoutingTable& o)
{
	os << o.info();
	return os;
}

}

