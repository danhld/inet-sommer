/** 
 * @short This module implements a sample routing protocol.
 * You can use it as a starting point for your own protocol.
 * 
 * Important: Please rename all occurrences of "sample" 
 * to something meaningful in the context of your protocol!
 * 
 *  @author Isabel Dietrich, isabel.dietrich@informatik.uni-erlangen.de
*/

#ifndef SAMPLE_ROUTING_H
#define SAMPLE_ROUTING_H

#include <omnetpp.h>
#include "SampleRoutingPkt_m.h"
#include "SampleUpperCtrlInfo_m.h"
#include "Ieee802Ctrl_m.h"
#include "IInterfaceTable.h"

class sampleRoutingProtocol : public cSimpleModule
{

	public:
		// LIFECYCLE

		virtual int     numInitStages() const {return 2;}
		virtual void    initialize(int);
		virtual void    finish();

		// OPERATIONS
		/** @brief Called every time a message arrives*/
		void handleMessage(cMessage*);


	private:  
		// OPERATIONS
		/** @brief Handle self messages such as timer... */
		virtual void HandleSelfMsg(cMessage*);

		/** @brief Handle messages from upper layer */
		virtual void HandleUpperMsg(cMessage*);

		/** @brief Handle messages from lower layer */
		virtual void HandleLowerMsg(cMessage*);

		/** @brief encapsulate packet */
		SampleRoutingPkt* encapsMsg(cMessage*);

		//@{
		/** @brief Sends a message to the lower layer */
		void SendDown(cMessage*);

		/** @brief Sends a message to the upper layer */
		void SendUp(cMessage*);
		//@}


		// MEMBER VARIABLES
		int mHeaderLength;

		/** @brief address of the node **/
		unsigned int myAddr;


		/** @brief gate id*/
		//@{
		int mUppergateIn;
		int mUppergateOut;
		int mLowergateIn;
		int mLowergateOut;
		//@}

};



#endif
