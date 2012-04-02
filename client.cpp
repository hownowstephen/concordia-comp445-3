// FTP Client over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247

#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

#include "protocol.cpp"
#include "socketlib.cpp"

FILE* tracefile = fopen("client.log","w");

int main(void){
    srand ( time(NULL) );

    //socket data types
    SOCKET client_socket;   // Client socket
    SOCKADDR_IN sa_out;      // fill with server info, IP, port

    char buffer[RAWBUF_SIZE]; // Buffer

    WSADATA wsadata;                                    // WSA connection
    char router[11];                                    // Host data
    char cusername[128], filename[128], direction[3];   // Other header data
    DWORD dwusername = sizeof(cusername);               // Retains the size of the username

    try {

        if (WSAStartup(0x0202,&wsadata)!=0){  
            throw "Error in starting WSAStartup";
        } else {

            /* Display the wsadata structure */
            cout<< endl
                << "wsadata.wVersion "       << wsadata.wVersion       << endl
                << "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
                << "wsadata.szDescription "  << wsadata.szDescription  << endl
                << "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
                << "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
                << "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
        }  

        client_socket = open_port(PEER_PORT2);

        prompt("Enter the router hostname: ",router);
        sa_out = prepare_peer_connection(router, ROUTER_PORT2);

        prompt("Enter a filename: ",filename);                  // Retrieve a filename from the client
        prompt("Direction of transfer [get|put]: ",direction);  // Retrieve a transfer direction

        // Make sure the direction is one of get or put
        if(!strcmp(direction,GET) || !strcmp(direction,PUT)){ 

            // Retrieve the local user name
            GetUserName(cusername,&dwusername);

            /*int selected = rand() % 256;
            int received, verify;

            int client_num = 0; // Client packet number
            int server_num = 0; // Server packet number

            int progress = 0;

            while(1){

                client_num = 3;
                // Send acknowledgement to the client along with our random number
                sprintf(buffer,"RAND %d",selected);
                cout << "Sending " << buffer << endl;
                if(sendbuf(client_socket, sa_out, &client_num, buffer, BUFFER_SIZE, true) < 0){
                if(progress < 1) continue;
                }else progress = 1;

                server_num = 1;
                // Finally wait for a response from the client with the number
                if(recvbuf(client_socket, sa_out, &server_num, buffer, BUFFER_SIZE, true) < 0){
                if(progress < 2) continue;
                }else progress = 2;
                cout << "Received " << buffer << endl;
                sscanf(buffer,"RAND %d %d",&verify,&received);

                client_num = 2;
                // Send acknowledgement to the client along with our random number
                sprintf(buffer,"RAND %d",received);
                cout << "Sending " << buffer << endl;
                if(sendbuf(client_socket, sa_out, &client_num, buffer, BUFFER_SIZE, true) < 0){
                if(progress < 3) continue;
                }else progress = 3;

                if(progress == 3) break;
            }

            client_num = selected & 0x1;
            server_num = received & 0x1;

            cout << "Starting with server packet " << server_num << " and client packet " << client_num << endl;
            */
            // Send client headers
            sprintf(buffer,HEADER, cusername, direction, filename); 
            send_packet(client_socket,sa_out,buffer,RAWBUF_SIZE,0);

            // Perform a get request
            if(!strcmp(direction,GET)){
                get(client_socket, sa_out, cusername, filename);
                
            }else if(!strcmp(direction,PUT)){
                put(client_socket, sa_out, cusername, filename);
            }

        }else{
            throw "The method you requested does not exist, use get or put";
        }

    } // try loop

    //Display any needed error response.
    catch (const char *str) { 
        cerr << str << WSAGetLastError() << endl;
    }

    fclose(tracefile);

    //close the client socket and clean up
    closesocket(client_socket);
    WSACleanup();  
    return 0;
}





