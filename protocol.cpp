#include "ftplib.cpp"

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define PACKET_SIZE 80      // Size (in bytes) of each packet
#define WINDOW_SIZE 1

/**
 * GET function
 * Performs the receiving half of a request
 */
void get(SOCKET s, SOCKADDR_IN sa, char * username, char* filename, FILE* logfile){

    char buffer[PACKET_SIZE * WINDOW_SIZE];
    int count, filesize;

    FILE* recv_file = fopen(filename, 'wb');

    recv_packet(s, sa, buffer, PACKET_SIZE); // Receives the filesize negotiation packet

    if(!strncmp())

    // Receive the file
    while(count < filesize){
        if(filesize - count >= BUFFER_SIZE) size = (sizeof(szbuffer) / sizeof(char)) - sizeof(char); // Read a full buffer
        else                                size = ((filesize - count) / sizeof(char)) - sizeof(char);  // Read a shorter buffer
        recv_frame(s,sa,&packet_num,buffer);
        fwrite(buffer,sizeof(char),size,recv_file);
        count += sizeof(buffer);
        cout << "Received " << count << " of " << filesize << " bytes" << endl;
    }
}

/**
 * PUT function
 * Performs the sending half of a request
 */
void put(SOCKET s, SOCKADDR_IN sa, char * username, char* filename, FILE* logfile){

    FILE* send_file = fopen(filename, 'rb');    // open the file
    char buffer[PACKET_SIZE * WINDOW_SIZE];     // initialize send buffer
    int filesize;
    int size = 0, sent = 0;                     // Trace variables

    // Determines the file size
    fseek(send_file, 0L, SEEK_END);
    filesize = ftell(send_file);
    fseek(send_file, 0L, SEEK_SET);

    cout << "File size: " << filesize << endl;

    buffer = "SIZ";
    memset(buffer + (4 * sizeof(char)), filesize, sizeof(int)); // Add the size of the element to the buffer
    send_packet(s,sa,buffer,PACKET_SIZE);

    cout << "Sending..." << buffer << endl;

    // Send the file
    int size = 0, sent = 0;
    // Loop through the file and stream in chunks based on the buffer size
    while ( !feof(send_file) ){
        fread(szbuffer, 0, sizeof(buffersend_file);
        send_frame(s,sa,&packet_num,szbuffer);
    }

}

