#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#define RAWBUF_SIZE 512

void prompt(const char* message, char*buffer){
    cout << message << flush ;  // Print the message
    cin >> buffer;              // Record the input into the buffer
}

/**
 * Open a port
 * Opens a local port for new connections
 */
SOCKET open_port(int port){
    SOCKET sock;      // Define the socket to return
    SOCKADDR_IN sa;     // Define the socket address information
    HOSTENT *hp;        // Host entity details
    char hostname[11]; // Store the value of localhost

    // Retrieve the local hostname
    gethostname(hostname,11);

    if((hp=gethostbyname(hostname)) == NULL)   throw "Could not determine a host address from supplied name";
    //Fill-in UDP Port and Address info.
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
     // Create the socket
    if((sock = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET) throw "Generating a new local socket failed";
    // Bind to the client port
    if (bind(sock,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR) throw "Could not bind socket to supplied port";

    return sock;
}

/**
 * Prepare a peer connection
 * Generates a socket connection object to a peer
 */
SOCKADDR_IN prepare_peer_connection(char* hostname, int port){
    SOCKADDR_IN sa;
    HOSTENT *hp;
    if((hp=gethostbyname(hostname)) == NULL) throw "Could not determine a host address from supplied name";

    cout << "Peer connection: " << hostname << ":" << port << endl;

    // Fill in port and address information
    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);
    sa.sin_family = hp->h_addrtype;   
    sa.sin_port = htons(port);
    return sa;
}