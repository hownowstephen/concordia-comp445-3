#include <iostream>

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define FRAME_SIZE 80      // Size (in bytes) of each packet
#define WINDOW_SIZE 1

void log_transaction(char* message, FILE* logfile){

}


/**
 * Create a packet with an identifier tag
 * Takes a buffer of size packet_size and generates a packet of size packet_size + sizeof(int)
 */
char* make_packet(char* packet, int packet_size, int number){
    char raw_packet[packet_size + sizeof(int)]; // Create a buffer to store the full packet
    memcpy(raw_packet, packet, packet_size);    // Copy the base packet into the full packet
    memcpy(raw_packet + packet_size, &number, sizeof( int ) );  // Copy the tag into the packet
    return raw_packet;
}

/**
 * Split a packet into packet name and identifier
 * Takes a packet of size packet_size + sizeof(int) and extracts a packet (size packet_size) and its integer identifier
 */
void split_packet(char* raw_packet, int packet_size, char* packet, int* number){
    memcpy(packet, raw_packet, packet_size);                // Extract the actual packet data
    memcpy(number, raw_packet + packet_size, sizeof(int));  // Extract the packet identifier
}

/**
 * Send Packet over a UDP socket
 * Generates a tagged packet and sends it over the supplied socket
 */
int send_packet(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    int ibytessent = 0;
    char* realbuf = make_packet(buffer, size, pid); // Convert to a tagged packet
    if ((ibytessent = send(sock,buffer,size + sizeof(int),0)) == SOCKET_ERROR){ 
        throw "Send failed"; 
    }else{
        memset(buffer,0,buffer_size);   // Zero the buffer
        return ibytessent;              // Return the number of sent bytes
    }   
}

/**
 * Receieve a packet over a UDP socket
 * Accepts a tagged packet over a socket and converts to packet data and tag
 */
int recv_packet(SOCKET sock, SOCKADDR_IN sa, char* buffer, int size, int pid){
    int ibytesrecv = 0;
    memset(buffer,0,buffer_size); // Clear the buffer to prepare to receive data
    char realbuf[size + sizeof(int)];
    if((ibytesrecv = recv(sock,realbuf,size + sizeof(int),0)) == SOCKET_ERROR){
        throw "Recv failed";
    }else{

        return ibytesrecv;  // Return the amount of data received
    }
}

/**
 * Send an entire frame over a udp socket
 * Loops over supplied char* buffer and sends the frame in a series of packets
 */
int send_frame(SOCKET sock, SOCKADDR_IN sa, char* frame, int frame_size, int window_size, int ack_id){
    int packet_size = frame_size / window_size; // Calculate the size of the packet
    char packet[packet_size];                   // Create the base packet buffer
    for(int i=0;i<window_size;i++){             // Loop over the window and send each packet
        strncpy(packet, from+(i*packet_size), packet_size); // Copy the packet information into the packet
        send_packet(sock, sa, packet, packet_size, i);      // Send a tagged packet over the supplied socket
    }
    char ack[packet_size + sizeof(int)];        // Create a buffer for the ack/nack
    recv_packet(sock, sa, ack, packet_size, 0); // Receive the acknowledgment
}

/**
 * Receive an entire frame over a udp socket
 */
int recv_frame(SOCKET sock, SOCKADDR_IN sa, char* frame, int frame_size, int window_size){
    int packet_size = frame_size / window_size; // Calculate the size of the packet
    char raw_packet[packet_size];   // Recv packet buffer
    char* packet;                   // Packet buffer
    int* pid;                       // Packet identifier
    for(int i=0;i<window_size;i++){
        recv_packet(sock, sa, packet, packet_size);
    }
    send_packet(sock, sa, "ACK", packet_size, 0);
}
