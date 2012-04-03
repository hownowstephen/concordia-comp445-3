#include "ftplib.cpp"

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define FRAME_SIZE 80      // Size (in bytes) of each packet
#define WINDOW_SIZE 1

#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define HEADER "%s\t%s\t%s" // Format string for headers

/**
 * GET function
 * Performs the receiving half of a request
 */
void get(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){
    int buffer_size = FRAME_SIZE * WINDOW_SIZE;
    char buffer[buffer_size];
    int count, filesize, size;

    FILE* recv_file = fopen(filename, "wb");

    recv_packet(s, sa, buffer, FRAME_SIZE, 0); // Receives the filesize negotiation packet

    memcpy(&filesize, buffer + (3 * sizeof(char)), sizeof(int));

    cout << "File size: " << filesize << endl;

    // Receive the file
    while(count + 1 < filesize){
        if(filesize - count >= (buffer_size))
            size = (sizeof(buffer) / sizeof(char)) - sizeof(char); // Read a full buffer
        else
            size = ((filesize - count) / sizeof(char)) - sizeof(char);  // Read a shorter buffer
        count += recv_packet(s,sa,buffer,buffer_size,0);
        fwrite(buffer,sizeof(char),size,recv_file);
        cout << "Received " << count << " of " << filesize << " bytes" << endl;
    }
    fclose(recv_file);
}

/**
 * PUT function
 * Performs the sending half of a request
 */
void put(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){

    int buffer_size = FRAME_SIZE * WINDOW_SIZE;
    char buffer[buffer_size];   // initialize send buffer
    int filesize;
    int size = 0, sent = 0;     // Trace variables

    FILE* send_file;

    if((send_file = fopen(filename, "rb")) != NULL){    // open the file

        // Determines the file size
        fseek(send_file, 0L, SEEK_END);
        filesize = ftell(send_file);
        fseek(send_file, 0L, SEEK_SET);

        cout << "File size: " << filesize << endl;

        strncpy(buffer, "SIZ", 3);
        memcpy(buffer + (3 * sizeof(char)), &filesize, sizeof(int)); // Add the size of the element to the buffer
        send_packet(s,sa,buffer,buffer_size,0);

        cout << "Sending..." << buffer << endl;

        memset(buffer, 0, sizeof(buffer));

        int count = 0;
        while (fgets(buffer, buffer_size, send_file) != NULL){
            count += send_packet(s,sa,buffer,buffer_size,0);
            cout << count << " bytes sent" << endl;
            memset(buffer, 0, sizeof(buffer));
        }

        fclose(send_file);
    }

}

