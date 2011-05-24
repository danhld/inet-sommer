// this is the definition of a sample message type that can be used
// to pass control information from one layer to the layer direcly below.
// In this case, it is used to pass the name of the destination host
// for a particular data message from the application to the sample routing protocol.

// You can use it as a starting point for your own messages.

// Important: Please rename all occurrences of "sample" 
// to something meaningful in the context of your protocol!

// author: Isabel Dietrich, isabel.dietrich@informatik.uni-erlangen.de

cplusplus {{

using namespace std;

}}



message SampleUpperCtrlInfo
{
    string destName;
}
