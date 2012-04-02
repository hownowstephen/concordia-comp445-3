#include <iostream>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

void log_transaction(char* message, FILE* logfile){

}

/**
 * Create a packet with an identifier tag
 * Takes a buffer of size packet_size and generates a packet of size packet_size + sizeof(int)
 */
void make_packet(char* buffer, char* packet, int packet_size, int number){
    memcpy(buffer, packet, packet_size);    // Copy the base packet into the full packet
    memcpy(buffer + packet_size, &number, sizeof( int ) );  // Copy the tag into the packet
}

/**
 * Split a packet into packet name and identifier
 * Takes a packet of size packet_size + sizeof(int) and extracts a packet (size packet_size) and its integer identifier
 */
void split_packet(char* buffer, int packet_size, char* packet, int* number){
    memcpy(packet, buffer, packet_size);                // Extract the actual packet data
    memcpy(number, buffer + packet_size, sizeof(int));  // Extract the packet identifier
}

/**
 * Send Packet over a UDP socket
 * Generates a tagged packet and sends it over the supplied socket
 */
int send_packet(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    int ibytessent = 0;
    int packet_size = size + sizeof(int);
    char packet[packet_size];
    make_packet(packet, buffer, size, pid); // Convert to a tagged packet
    cout << "SENDING " << packet << endl;
    if ((ibytessent = send(sock,buffer,size,0)) == SOCKET_ERROR){ 
        throw "Send failed"; 
    }else{
        memset(buffer,0,size);  // Zero the buffer
        return ibytessent;      // Return the number of sent bytes
    }   
}

/**
 * Receieve a packet over a UDP socket
 * Accepts a tagged packet over a socket and converts to packet data and tag
 */
int recv_packet(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    int ibytesrecv = 0;
    memset(buffer,0,size); // Clear the buffer to prepare to receive data
    char packet[size + sizeof(int)];
    if((ibytesrecv = recv(sock,packet,size + sizeof(int),0)) == SOCKET_ERROR){
        throw "Recv failed";
    }else{
        int* packet_id;
        split_packet(packet, size, buffer, packet_id);
        if(pid == *packet_id){
            return ibytesrecv;  // Return the amount of data received
        }else{
            return -1 * (*packet_id);   // Return the negation of the packet id actually received
        }
    }
}

/**
 * Send an entire frame over a udp socket
 * Loops over supplied char* buffer and sends the frame in a series of packets
 */
int send_frame(SOCKET sock, SOCKADDR_IN sa, char* frame, int frame_size, int window_size){
    int packet_size = frame_size / window_size; // Calculate the size of the packet
    char packet[packet_size];                   // Create the base packet buffer
    for(int i=0;i<window_size;i++){
        strncpy(packet, frame+(i*packet_size), packet_size); // Copy the packet information into the packet
        send_packet(sock, sa, packet, packet_size, i);      // Send a tagged packet over the supplied socket
    }
    char ack[packet_size + sizeof(int)];        // Create a buffer for the ack/nack
    recv_packet(sock, sa, ack, packet_size, 0); // Receive the acknowledgment
}

/**
 * Receive an entire frame over a udp socket
 * Loops over the expected packet ids and performs a recv
 */
int recv_frame(SOCKET sock, SOCKADDR_IN sa, char* frame, int frame_size, int window_size){
    int packet_size = frame_size / window_size; // Calculate the size of the packet
    char raw_packet[packet_size];               // Recv packet buffer
    char* packet;                               // Packet buffer
    int* pid;                                   // Packet identifier
    int recv;                                   // Recv data from recv_packet
    for(int i=0;i<window_size;i++){
        if((recv = recv_packet(sock, sa, packet, packet_size, i)) < 0){
            cout << "Received the wrong packet, expecting " << i << " got " << recv << endl;
            memset(packet,0,packet_size);   // Zero the buffer
            packet = "NAK";
            memset(packet + (3*sizeof(char)), abs(recv), sizeof(int)); // Send the NAK with the packet ID
            send_packet(sock, sa, "NAK", packet_size, 0);
        }else{
            strncpy(frame+(i*packet_size), packet, packet_size);    // Copy the buffer into the frame output
        }
    }
    send_packet(sock, sa, "ACK", packet_size, 0); // Send the ACK for this frame
}
