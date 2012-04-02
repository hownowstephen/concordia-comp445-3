#include "ftplib.cpp"

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define PACKET_SIZE 80      // Size (in bytes) of each packet
#define WINDOW_SIZE 1

#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define HEADER "%s\t%s\t%s" // Format string for headers

/**
 * GET function
 * Performs the receiving half of a request
 */
void get(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){
    int buffer_size = PACKET_SIZE * WINDOW_SIZE;
    char buffer[buffer_size];
    int count, filesize, size;

    FILE* recv_file = fopen(filename, "wb");

    recv_packet(s, sa, buffer, PACKET_SIZE, 0); // Receives the filesize negotiation packet

    //if(!strncmp())

    // Receive the file
    while(count < filesize){
        if(filesize - count >= (buffer_size)) size = (sizeof(buffer) / sizeof(char)) - sizeof(char); // Read a full buffer
        else                                size = ((filesize - count) / sizeof(char)) - sizeof(char);  // Read a shorter buffer
        recv_frame(s,sa,buffer,buffer_size,WINDOW_SIZE);
        fwrite(buffer,sizeof(char),size,recv_file);
        count += sizeof(buffer);
        cout << "Received " << count << " of " << filesize << " bytes" << endl;
    }
    fclose(recv_file);
}

/**
 * PUT function
 * Performs the sending half of a request
 */
void put(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){

    int buffer_size = PACKET_SIZE * WINDOW_SIZE;
    char buffer[buffer_size];   // initialize send buffer
    int filesize;
    int size = 0, sent = 0;     // Trace variables

    FILE* send_file = fopen(filename, "rb");    // open the file

    // Determines the file size
    fseek(send_file, 0L, SEEK_END);
    filesize = ftell(send_file);
    fseek(send_file, 0L, SEEK_SET);

    cout << "File size: " << filesize << endl;

    strncpy(buffer, "SIZ", 3);
    memset(buffer + (4 * sizeof(char)), filesize, sizeof(int)); // Add the size of the element to the buffer
    send_packet(s,sa,buffer,buffer_size,WINDOW_SIZE);

    cout << "Sending..." << buffer << endl;

    // Loop through the file and stream in chunks based on the buffer size
    while ( !feof(send_file) ){
        fread(buffer, 0, sizeof(buffer), send_file);
        send_frame(s,sa,buffer,sizeof(buffer),0);
    }

    fclose(send_file);

}

