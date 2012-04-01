//Router.cpp
#include "Router.h"
//////////////////////////////////////////////////////////
//
//  Router Constructor
//  arguements:
//      fn: A string of log file name
//
//////////////////////////////////////////////////////////

Router::Router(char *fn)        //Constructor
{
    WSADATA wsadata;
    HOSTENT* hp;                
    char peer_name1[MAXHOSTNAMELEN], peer_name2[MAXHOSTNAMELEN];
    SOCKADDR_IN sa_in;

    FileBuf.empty=true;

    try 
    {                  
        if (WSAStartup(0x0202,&wsadata)!=0)
            throw "Error in starting WSAStartup()\n";
    }

    //Display any needed error response.
    catch (char *str) { cerr<<str<<":"<<dec<<WSAGetLastError()<<endl; return;}

    //Get Host name
    gethostname(localhost,MAXHOSTNAMELEN);
    cout<<"Router starting on host:"<<localhost<<endl<<flush;

    try
    {
        //Create the Udp Sock1
        if((Sock1 = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET) 
            throw "Create UDP Socket1 failed\n";

        //Fill-in UDP Port and Address info.
        sa_in.sin_family = AF_INET;
        sa_in.sin_port = htons(ROUTER_PORT1);
        sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

        //Bind the UDP port1
        if (bind(Sock1,(LPSOCKADDR)&sa_in,sizeof(sa_in)) == SOCKET_ERROR)
            throw "can't bind the socket1";

        //Create the Udp Sock2
        if((Sock2 = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET) 
            throw "Create UDP Socket2 failed\n";

        //Fill-in UDP Port and Address info.
        sa_in.sin_family = AF_INET;
        sa_in.sin_port = htons(ROUTER_PORT2);
        sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

        //Bind the UDP port2
        if (bind(Sock2,(LPSOCKADDR)&sa_in,sizeof(sa_in)) == SOCKET_ERROR)
            throw "can't bind the socket2";

        cout<<"\nPlease enter the first peer host name:"<<flush;        //enter the dropping rate.
        cin>>peer_name1;
        cout<<"\nPlease enter the second peer host name:"<<flush;       //enter the dropping rate.
        cin>>peer_name2;
        cout<<"\nPlease enter the drop rate:"<<flush;       //enter the dropping rate.
        cin>>damage_rate;
        cout<<"\nPlease enter the delay rate:"<<flush;      //enter the dropping rate.
        cin>>delay_rate;

        //creat peer host1
        if((hp=gethostbyname(peer_name1)) == NULL) 
            throw "get server name failed\n";
        memset(&sa_in_peer1,0,sizeof(sa_in_peer1));
        memcpy(&sa_in_peer1.sin_addr,hp->h_addr,hp->h_length);
        sa_in_peer1.sin_family = hp->h_addrtype;   
        sa_in_peer1.sin_port = htons(PEER_PORT1);

        //creat peer host2
        if((hp=gethostbyname(peer_name2)) == NULL) 
            throw "get client name failed\n";
        memset(&sa_in_peer2,0,sizeof(sa_in_peer2));
        memcpy(&sa_in_peer2.sin_addr,hp->h_addr,hp->h_length);
        sa_in_peer2.sin_family = hp->h_addrtype;   
        sa_in_peer2.sin_port = htons(PEER_PORT2);

    }       
    catch (const char *str) { cerr << str << ":" << dec << WSAGetLastError() << endl; exit(1); }

    srand( (unsigned)time( NULL ) );
}

//////////////////////////////////////////////////////////
//
//  Router::IsDamage
//      The function that generates random damages according to damage rate.
//
//////////////////////////////////////////////////////////

bool Router::IsDamage() const
{   
    return ( (((float)rand())/RAND_MAX) < ((float)damage_rate/100));
}

//////////////////////////////////////////////////////////
//
//  Router::IsDelayed
//      The function that generates random delayed according to delay rate.
//
//////////////////////////////////////////////////////////

bool Router::IsDelayed() const
{   
    return ( (((float)rand())/RAND_MAX) < ((float)delay_rate/100));
}

//////////////////////////////////////////////////////////
//
//  Router::Run
//      The function receives packets from peer hosts and forwards to destinations.  
//      It also drops packets and stores delayed packets for future sending.
//      It calls SendProc to send delayed packets.
//
//////////////////////////////////////////////////////////

void Router::Run()
{
    fd_set readfds;
    struct timeval *tp=new timeval;
    SOCKADDR from;
    int RetVal, fromlen, recvlen, wait_count;
    EVENT_LIST temp;
    DWORD CurrentTime, count1, count2;

    count1=0; 
    count2=0;
    wait_count=0;
    tp->tv_sec=0;
    tp->tv_usec=TIMEOUT_USEC;

    while (1)
    {
        try
        {
            FD_ZERO(&readfds);
            FD_SET(Sock1,&readfds);
            FD_SET(Sock2,&readfds);
            fromlen=sizeof(from);
            if((RetVal=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR)  //check for incoming packets.
                throw "Timer error!";
            else if(RetVal>0)   //There are incoming packets.
            {
                if(!FileBuf.empty) wait_count++;
                if(FD_ISSET(Sock1, &readfds))   //incoming packet from peer host 1
                {
                    if((recvlen=recvfrom(Sock1, temp.Buffer, sizeof(temp.Buffer), 0, &from, &fromlen))==SOCKET_ERROR)
                        throw " Get buffer error!";
                    if (TRACE)
                    {
                        cout<<"Router: Receive packet "<<count1<<" from peer host 1"<<endl;
                    }
                    temp.count=count1;
                    count1++;
                    temp.destination=2;
                }
                else if(FD_ISSET(Sock2, &readfds))  //incoming packet from peer host 2
                {
                    if((recvlen=recvfrom(Sock2, temp.Buffer, sizeof(temp.Buffer), 0, &from, &fromlen))==SOCKET_ERROR)
                        throw " Get buffer error!";
                    if (TRACE)
                    {
                        cout<<"Router: Receive packet "<<count2<<" from peer host 2"<<endl;
                    }
                    temp.count=count2;
                    count2++;
                    temp.destination=1;
                }
                else continue;
                temp.len=recvlen;
                CurrentTime=GetTickCount();
                if(FileBuf.empty&&IsDelayed())      //if the packet is delayed.
                {
                    FileBuf=temp;
                    FileBuf.empty=false;
                    if (TRACE)
                    {
                        cout<<"Router: Packet "<<temp.count<<" received from peer host "<<(temp.destination==1?2:1)<<" has been delayed!"<<endl;
                    }
                }
                else if(IsDamage()) //if the packet is dropped: dropping packet by no forwarding the packet.
                {
                    if (TRACE)
                    {
                        cout<<"Router: Packet "<<temp.count<<" received from peer host "<<(temp.destination==1?2:1)<<" has been dropped by router!"<<endl;
                    }
                }
                else        //otherwise, packet is forwarded to destination
                {
                    if(temp.destination==1) //forward packets received from 2 to 1.
                    {
                        if(sendto(Sock1, temp.Buffer, temp.len,0,(SOCKADDR*)&sa_in_peer1,sizeof(sa_in_peer1))==SOCKET_ERROR)
                            throw "Send packet error!";
                        if (TRACE)
                        {
                            cout<<"Router: Send packet "<<temp.count<<" received from peer host "<<(temp.destination==1?2:1) <<" to host "<<temp.destination<<endl;
                        }
                        if(!FileBuf.empty&&FileBuf.destination==1)
                        {
                            wait_count=0;
                            SendProc();
                        }
                    }
                    else
                    {   //forward packets received from 1 to 2.
                        if(sendto(Sock2, temp.Buffer, temp.len,0,(SOCKADDR*)&sa_in_peer2,sizeof(sa_in_peer2))==SOCKET_ERROR)
                            throw "Send packet error1";
                        if (TRACE)
                        {
                            cout<<"Router: Send packet "<<temp.count<<" received from peer host "<<(temp.destination==1?2:1) <<" to host "<<temp.destination<<endl;
                        }
                        if(!FileBuf.empty&&FileBuf.destination==2)
                        {
                            wait_count=0;
                            SendProc();
                        }
                    }   
                }
            }
            else //If there is no incoming packet and there is a delayed packets storing in buffer for 3 cycle times (about 0.9 second), call SendProc to send delayed packet.
            {       
                if(!FileBuf.empty)
                {
                    wait_count++;
                    if(wait_count>=3)
                    {
                        SendProc();
                        wait_count=0;
                    }
                }
            }
        } //end of try
        catch(const char *str) {cerr<<str<<":"<<dec<<WSAGetLastError()<<endl;}
    }//end of while
}

//////////////////////////////////////////////////////////
//
//  Router::SendProc
//      Send delayed packets to the destinations.
//
//////////////////////////////////////////////////////////

void Router::SendProc()
{   
    cout << "Sending packet" << endl;
    try
    {
        if(FileBuf.destination==1)
        {
            if(sendto(Sock1, FileBuf.Buffer, FileBuf.len,0,(SOCKADDR*)&sa_in_peer1,sizeof(sa_in_peer1))==SOCKET_ERROR)
                throw "Send packet error!";
        }
        else
        {
            if(sendto(Sock2, FileBuf.Buffer, FileBuf.len,0,(SOCKADDR*)&sa_in_peer2,sizeof(sa_in_peer2))==SOCKET_ERROR)
                throw "Send packet error!";
        }
        if (TRACE)
        {
            cout<<"Router: Send delayed packet "<<FileBuf.count<<" received from peer host "<<(FileBuf.destination==1?2:1)<<" to host "<<FileBuf.destination<<endl;
        }
    }
    catch(const char *str){cerr<<str<<":"<<dec<<WSAGetLastError()<<endl;}
    FileBuf.empty=true;
}

//////////////////////////////////////////////////////////
//
//  Router Destructor
//  arguements:
//      fn: A string of log file name
//
//////////////////////////////////////////////////////////

Router :: ~Router()
{
    closesocket(Sock1);
    closesocket(Sock2);

    /* When done, uninstall winsock.dll (WSACleanup()) and exit */ 
    WSACleanup();  

}

//////////////////////////////////////////////////////////
//
//  Main function
//
//////////////////////////////////////////////////////////

int main()
{
    Router router;
    router.Run();
    return 0;
}
