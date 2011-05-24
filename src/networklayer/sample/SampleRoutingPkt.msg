// this is the definition of a sample message type that can be used
// to exchange data between instances of a protocol, for example
// route requests, route replies, route errors, other data messages,...

// You can use it as a starting point for your own messages.

// Important: Please rename all occurrences of "sample" 
// to something meaningful in the context of your protocol!

// author: Isabel Dietrich, isabel.dietrich@informatik.uni-erlangen.de

cplusplus {{

using namespace std;

}}


// insert fields as needed by your protocol
packet SampleRoutingPkt
{
    int destAddress;
    int srcAddress;
    simtime_t creationTime;
}
