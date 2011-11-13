/***********************************************************************
 basic-client.cpp - Implements a fairly basic Winsock client program
    that initiates a blocking TCP connection to an echo server, sends 
    some data, waits for and receives the reply, and dies.

    It also can delay a bit before shutting down so you can start
    multiple clients to test server connection handling.  The
    alternative is writing a single program that can make more than
    one connection, which would turn "basic-client" into something else
    entirely.

 Compiling:
    VC++: cl -GX basic-client.cpp main.cpp ws-util.cpp wsock32.lib
    BC++: bcc32 basic-client.cpp main.cpp ws-util.cpp
    
 This program is hereby released into the public domain.  There is
 ABSOLUTELY NO WARRANTY WHATSOEVER for this product.  Caveat hacker.
***********************************************************************/

#include "ws-util.h"

#include <winsock.h>

#include <iostream>
#include <string.h>

using namespace std;

// Comment this out to disable the shutdown-delay functionality.
#define SHUTDOWN_DELAY


////////////////////////////////////////////////////////////////////////
// Constants

// kBufferSize must be larger than the length of kpcEchoMessage.
const int kBufferSize = 1024;
const char* kpcEchoMessage = "This is a test of the emergency data "
        "transfer system.  If this had been real a real emergency, we "
        "would have sent this data out-of-band.";
const int kEchoMessageLen = strlen(kpcEchoMessage);

#if defined(SHUTDOWN_DELAY)
// How long to wait after we do the echo before shutting the connection
// down, to give the user time to start other clients, for testing 
// multiple simultaneous connections.
const int kShutdownDelay = 3;
#endif
        

////////////////////////////////////////////////////////////////////////
// Prototypes

u_long LookupAddress(const char* pcHost);
SOCKET EstablishConnection(u_long nRemoteAddr, u_short nPort);
bool SendEcho(SOCKET sd);
int ReadReply(SOCKET sd);


//// DoWinsock /////////////////////////////////////////////////////////
// The module's driver function -- we just call other functions and
// interpret their results.

int DoWinsock(const char* pcHost, int nPort)
{
    // Find the server's address
    cout << "Looking up address..." << flush;
    u_long nRemoteAddress = LookupAddress(pcHost);
    if (nRemoteAddress == INADDR_NONE) {
        cerr << endl << WSAGetLastErrorMessage("lookup address") << 
                endl;
        return 3;
    }
    in_addr Address;
    memcpy(&Address, &nRemoteAddress, sizeof(u_long)); 
    cout << inet_ntoa(Address) << ":" << nPort << endl; 

    // Connect to the server
    cout << "Connecting to remote host..." << flush;
    SOCKET sd = EstablishConnection(nRemoteAddress, htons(nPort));
    if (sd == INVALID_SOCKET) {
        cerr << endl << WSAGetLastErrorMessage("connect to server") << 
                endl;
        return 3;
    }
    cout << "connected, socket " << sd << "." << endl;

    // Send the echo packet to the server
    cout << "Sending echo packet (" << strlen(kpcEchoMessage) << " bytes)..." << flush;
    int nBytes;
    if (SendEcho(sd)) {
		cout << endl;
		if ((nBytes = ReadReply(sd)) > 0) {
			if (nBytes == kBufferSize) {
				cerr << "FYI, likely data overflow." << endl;
			}
		}
		else if (nBytes == 0) {
			cerr << endl << "Server unexpectedly closed the connection" <<
					endl;
		}
		else {
			cerr << endl << WSAGetLastErrorMessage("read reply") <<
					endl;
			return 3;
		}
	}
	else {
        cerr << endl << WSAGetLastErrorMessage("send echo packet") <<
                endl;
        return 3;
    }

#if defined(SHUTDOWN_DELAY)
    // Delay for a bit, so we can start other clients.  This is strictly
    // for testing purposes, so you can convince yourself that the 
    // server is handling more than one connection at a time.
    cout << "Will shut down in " << kShutdownDelay << 
            " seconds... (one dot per second): " << flush;
    for (int i = 0; i < kShutdownDelay; ++i) {
        Sleep(1000);
        cout << '.' << flush;
    }
    cout << endl;
#endif

    // Shut connection down
    cout << "Shutting connection down..." << flush;
    if (ShutdownConnection(sd)) {
        cout << "Connection is down." << endl;
    }
    else {
        cout << endl << WSAGetLastErrorMessage("Shutdown connection") <<
                endl;
    }

    cout << "All done!" << endl;

    return 0;
}


//// LookupAddress /////////////////////////////////////////////////////
// Given an address string, determine if it's a dotted-quad IP address
// or a domain address.  If the latter, ask DNS to resolve it.  In
// either case, return resolved IP address.  If we fail, we return
// INADDR_NONE.

u_long LookupAddress(const char* pcHost)
{
    u_long nRemoteAddr = inet_addr(pcHost);
    if (nRemoteAddr == INADDR_NONE) {
        // pcHost isn't a dotted IP, so resolve it through DNS
        hostent* pHE = gethostbyname(pcHost);
        if (pHE == 0) {
            return INADDR_NONE;
        }
        nRemoteAddr = *((u_long*)pHE->h_addr_list[0]);
    }

    return nRemoteAddr;
}


//// EstablishConnection ///////////////////////////////////////////////
// Connects to a given address, on a given port, both of which must be
// in network byte order.  Returns newly-connected socket if we succeed,
// or INVALID_SOCKET if we fail.

SOCKET EstablishConnection(u_long nRemoteAddr, u_short nPort)
{
    // Create a stream socket
    SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd != INVALID_SOCKET) {
        sockaddr_in sinRemote;
        sinRemote.sin_family = AF_INET;
        sinRemote.sin_addr.s_addr = nRemoteAddr;
        sinRemote.sin_port = nPort;
        if (connect(sd, (sockaddr*)&sinRemote, sizeof(sockaddr_in)) ==
                SOCKET_ERROR) {
            sd = INVALID_SOCKET;
        }
    }

    return sd;
}


//// SendEcho //////////////////////////////////////////////////////////
// Sends the echo packet to the server.  Returns true on success,
// false otherwise.

bool SendEcho(SOCKET sd)
{
    // Send the string to the server
    if (send(sd, kpcEchoMessage, kEchoMessageLen, 0) != SOCKET_ERROR) {
		return true;
	}
	else {
        return false;
    }
}


//// ReadReply /////////////////////////////////////////////////////////
// Read the reply packet and check it for sanity.  Returns -1 on 
// error, 0 on connection closed, > 0 on success.

int ReadReply(SOCKET sd)
{
    // Read reply from server
    char acReadBuffer[kBufferSize];
    int nTotalBytes = 0;
    while (nTotalBytes < kEchoMessageLen) {
        int nNewBytes = recv(sd, acReadBuffer + nTotalBytes, 
                kBufferSize - nTotalBytes, 0);
        if (nNewBytes == SOCKET_ERROR) {
            return -1;
        }
        else if (nNewBytes == 0) {
            cerr << "Connection closed by peer." << endl;
            return 0;
        }

        nTotalBytes += nNewBytes;
    }

    // Check data for sanity
    if (strncmp(acReadBuffer, kpcEchoMessage, nTotalBytes) == 0) {
		cout << "Reply packet matches what we sent!" << endl;
	}
	else {
        cerr << "Mismatch in data received from server. " <<
                "Something's broken!" << endl;
    }

    return nTotalBytes;
}

