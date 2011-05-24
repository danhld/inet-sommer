/**
 * @short Example implementation of a Traffic Generator derived from the
          TrafGen base class
 * @author Isabel Dietrich
*/

#ifndef SAMPLE_APP
#define SAMPLE_APP

#include "TrafGen.h"

class sampleApp : public TrafGen
{
public:

    // LIFECYCLE
    // this takes care of constructors and destructors

	virtual void initialize(int);
	virtual void finish();
  
protected:

  // OPERATIONS
	virtual void handleSelfMsg(cMessage*);
	virtual void handleLowerMsg(cMessage*);
	
	virtual void SendTraf(cPacket *msg, const char *dest);

private:    
    
	int mLowergateIn;
	int mLowergateOut;
    
    int mCurrentTrafficPattern;
	
	cOutVector msgSentVec;
	
	double mNumTrafficMsgs;


};

#endif
