// FTP Server over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247

#include <winsock.h>
#include <iostream>
#include <time.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "protocol.cpp"
#include "socketlib.cpp"

int main(int argc, char **argv){
    /* Main function, performs the listening loop for client connections */
    srand ( time(NULL) );

    SOCKET server_socket;       // Global listening socket
    SOCKADDR_IN sa_out;         // fill with router info
    WSADATA wsadata;            // Winsock connection object
    char router[11];            // Store the name of the router
    char* trace_data;

    FILE* logfile = fopen("server.log", 'w');
     
    try {
        if (WSAStartup(0x0202,&wsadata)!=0){  
            throw "Error in starting WSAStartup";
        }else{
            /* display the wsadata structure */
            cout<< endl
                << "wsadata.wVersion "       << wsadata.wVersion       << endl
                << "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
                << "wsadata.szDescription "  << wsadata.szDescription  << endl
                << "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
                << "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
                << "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
        }
        
        server_socket = open_port(PEER_PORT1);
        // Prompt for router connection
        prompt("Enter the router hostname: ", router);
        sa_out = prepare_peer_connection(router, ROUTER_PORT1);

        // Server will block waiting for new client requests indefinitely
        while(1){

            char buffer[RAWBUF_SIZE]; // buffer object
            int server_num = 0;         // client packet tracer
            int client_num = 0;         // server packet tracer

            int selected = rand() % 256;
            int received, verify;

            int progress = 0;
            int rcv;

            while(1){

                if(progress < 1){
                    // Receive a random number from the client
                    if(recv_safe(server_socket, sa_out, buffer, RAWBUF_SIZE, 200) == 200){
                        cout << "Received " << buffer << endl;
                        sscanf(buffer,"RAND %d",&received);
                    }else continue;

                    // Send acknowledgement to the client along with our random number
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer,"RAND %d %d",received,selected);
                    if(send_safe(server_socket, sa_out, buffer, RAWBUF_SIZE, 100) != 100){
                        cout << "Sent " << buffer << endl;
                        continue;
                    }
                    progress = 1;
                }

                // Finally wait for a response from the client with the number
                if((rcv = recv_safe(server_socket, sa_out, buffer, RAWBUF_SIZE, 201)) == 201){
                    cout << "Received " << buffer << endl;
                    sscanf(buffer,"RAND %d",&verify);
                    break;
                }else if(rcv == 200){
                    progress = 0;
                    continue;
                }
            }

            client_num = received % WINDOW_SIZE + 1;
            server_num = selected % WINDOW_SIZE + 1;

            memset(trace_data, 0, sizeof(trace_data));
            sprintf(trace_data, "negotiated srv %d and cli %d", server_num, client_num);
            trace(logfile, "SERVER", trace_data);

            // Receive header data from the client
            if(recv_safe(server_socket, sa_out, buffer, RAWBUF_SIZE, 777) == 777){

                // Extract data from the headers
                char cusername[128], filename[128], direction[3];
                sscanf(buffer,HEADER,cusername,direction,filename);

                // Print out the information
                memset(trace_data, 0, sizeof(trace_data));
                sprintf(trace_data, "client %s requesting %s of %s", cusername, direction, filename)
                trace(logfile, "SERVER", trace_data);

                // Respond to the client request
                if(!strcmp(direction,GET)){
                    put(server_socket, sa_out, "SERVER", filename, client_num, server_num, logfile);
                }else if(!strcmp(direction,PUT)){
                    get(server_socket, sa_out, "SERVER", filename, client_num, server_num, logfile);
                }else   throw "Requested protocol does not exist";
            }
        }

    // Catch and print any errors
    } catch(const char * str){
        cerr << str << WSAGetLastError() << endl;
    }

    //close server socket and clean up the winsock
    closesocket(server_socket);
    WSACleanup();
    return 0;
}




