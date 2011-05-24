package inet.networklayer.sample;

simple sampleRoutingProtocol
{
    parameters:
        bool coreDebug; // debug switch for the core framework
        double headerLengthByte; // length of the protocol header (in byte)
    gates:
        input uppergateIn; // from the network layer
        output uppergateOut; // to the network layer
        input ifIn; // from the physical layer
        output ifOut; // to the physical layer
}
