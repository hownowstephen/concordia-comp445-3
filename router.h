
//Router Head file
#ifndef ROUTER_H
#define ROUTER_H

#include <winsock.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <list>
#include <stdio.h>

using namespace std ;

#define MAXBUFSIZE 2048             //maximum packet size
#define MAXHOSTNAMELEN 256          //maximum length of host name
#define ROUTER_PORT1 7000           //router port number 1
#define ROUTER_PORT2 7001           //router port number 2
#define PEER_PORT1 5000             //port number of peer host 1
#define PEER_PORT2 5001             //port number of peer host 2
#define TIMEOUT_USEC 300000         //time-out value

#define TRACE 1

struct EVENT_LIST
{
    bool empty;
    DWORD count;                    //count is the packet number
    short destination;              //destination of this packet
    int len;                        //length of this packet
    char Buffer[MAXBUFSIZE];        //buffer for packet
};

class Router
{
public:
    char localhost[MAXHOSTNAMELEN];     //local host name
    //Constructor
    Router(char *fn="log.txt");

    //Destructor
    ~Router();

    void Run();

private:
    float damage_rate, delay_rate;              //damage rate: dropped and delayed
    SOCKET Sock1, Sock2;            //sockets used for communcation with peer host 1 and 2
    EVENT_LIST FileBuf;     //buffer for delayed packets

protected:
    SOCKADDR_IN sa_in_peer1;        // address structure for peer host 1 address
    SOCKADDR_IN sa_in_peer2;        // address structure for peer host 2 address
    
    bool IsDamage() const;
    bool IsDelayed() const;
    void SendProc();
};

#endif