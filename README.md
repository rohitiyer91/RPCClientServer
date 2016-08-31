# RPCClientServer
This project provide a basic RPC(Remote Procedure Call). Since this requires marshalling and un-marshalling for the incoming and outgoing message, the same support has been added to the above code. The server portion is implemented by RPCServer.c, by intiailizing a UDP server at a port to be specified by the user at runtime. The server will listed to all incoming connections on this port and handle marshalling/un-marshalling of the messages to and from the server.

The client portion is implemented by RPClient.c again talking to the server on a prespecified port. Since this is UDP, the client wil not wait for any response from the server.
