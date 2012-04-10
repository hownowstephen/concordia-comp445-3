#include <iostream>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <Math.h>

using namespace std;

#define TIMEOUT_USEC 300000

void write_log(FILE* logfile, char* username, char* message){
    fprintf(logfile, "%s > %s\n", username, message);
    memset(message, 0, sizeof(message));
}

/**
 * Create a packet with an identifier tag
 * Takes a buffer of size packet_size and generates a packet of size packet_size + sizeof(int)
 */
void make_packet(char* packet, char* buffer, int buffer_size, int number){
    memcpy(packet, buffer, buffer_size);    // Copy the base packet into the full packet
    memcpy(packet + buffer_size, &number, sizeof( int ) );  // Copy the tag into the packet
}

/**
 * Split a packet into packet name and identifier
 * Takes a packet of size packet_size + sizeof(int) and extracts a packet (size packet_size) and its integer identifier
 */
void split_packet(char* packet, char* buffer, int buffer_size, int* number){
    memcpy(buffer, packet, buffer_size);                // Extract the actual packet data
    memcpy(number, packet + buffer_size, sizeof(int));  // Extract the packet identifier
}

/**
 * Send Packet over a UDP socket
 * Generates a tagged packet and sends it over the supplied socket
 */
int send_packet(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    int ibytessent = 0;
    int packet_size = size + sizeof(int);
    int from = sizeof(sa);  // Size of the sockaddr
    char packet[packet_size];
    make_packet(packet, buffer, size, pid); // Convert to a tagged packet
    if ((ibytessent = sendto(sock,packet,packet_size,0,(SOCKADDR*)&sa, from)) == SOCKET_ERROR){
        throw "Send failed"; 
    }else{
        memset(buffer,0,size);  // Zero the buffer
        return ibytessent - sizeof(int);      // Return the number of sent bytes
    }   
}

/**
 * Receieve a packet over a UDP socket
 * Accepts a tagged packet over a socket and converts to packet data and tag
 */
int recv_packet(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    int ibytesrecv, result;
    int from = sizeof(sa);
    int packet_size = size + sizeof(int);
    char packet[packet_size];
    fd_set readfds;                   // Used by select to manage file descriptor multiplexing

    struct timeval *tp=new timeval;   // Timeout struct
    tp->tv_sec=0;                     // Set current time
    tp->tv_usec=TIMEOUT_USEC;         // Set timeout time

    FD_ZERO(&readfds);
    FD_SET(sock,&readfds);
    if((result=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR) throw "Timer error!";
    else if(result > 0){
        memset(packet,0,packet_size);
        if((ibytesrecv = recvfrom(sock, packet, packet_size,0,(SOCKADDR*)&sa, &from)) == SOCKET_ERROR){
            throw "Recv failed";
        }else{
            int packet_id;
            memset(buffer,0,size); // Clear the buffer to prepare to receive data
            split_packet(packet, buffer, size, &packet_id);
            return packet_id;
        }
    }else{
        return -1;
    }
}


// Performs a safe send, loops sending and then sends an ack
int send_safe(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    char ackbuf[size];
    memset(ackbuf, 0, sizeof(ackbuf));
    send_packet(sock, sa, buffer, size, pid);
    return recv_packet(sock, sa, ackbuf, size, pid);
}


// Performs a safe recv function, only sends the WOOT ack on success
int recv_safe(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    char ackbuf[size];
    memset(ackbuf, 0, sizeof(ackbuf));
    strncpy(ackbuf, "WOOT", 4);
    int result;
    if((result = recv_packet(sock, sa, buffer, size, pid)) == pid)
        send_packet(sock, sa, ackbuf, size, pid);
    return result;
}
