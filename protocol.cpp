#include "ftplib.cpp"
#include <signal.h>

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define FRAME_SIZE  128      // Size (in bytes) of each packet
#define WINDOW_SIZE 1

#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define HEADER "%s\t%s\t%s" // Format string for headers


/**
 * Timeout functions
 * taken from http://users.encs.concordia.ca/~mia/tutorials/comp445/Go-Back-N/timer.html
 */
void setTimer(void){
  struct itimerval waitITV;
  struct timeval waitTV;
  waitTV.tv_sec = 0;
  waitTV.tv_usec = 110;
  waitITV.it_interval = waitTV;
  waitITV.it_value = waitTV;
  setitimer(ITIMER_REAL, &waitITV, NULL);
}

void unsetTimer(void){
  setitimer(ITIMER_REAL, NULL, NULL);
}

/**
 * GET function
 * Performs the receiving half of a request
 */
void get(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){
    int FRAME_SIZE = FRAME_SIZE;
    char buffer[FRAME_SIZE];
    int count, filesize, size;

    FILE* recv_file = fopen(filename, "wb");

    recv_packet(s, sa, buffer, FRAME_SIZE, 0); // Receives the filesize negotiation packet

    memcpy(&filesize, buffer + (3 * sizeof(char)), sizeof(int));

    cout << "File size: " << filesize << endl;

    count = 0;
    // Receive the file
    while(count < filesize){
        if(filesize - count >= (FRAME_SIZE))    size = (FRAME_SIZE / sizeof(char));         // Read the full buffer
        else                                    size = ((filesize - count) / sizeof(char)); // Read a subset of the buffer
        count += recv_packet(s,sa,buffer,FRAME_SIZE,0); // Receive the packet from the peer
        fwrite(buffer,sizeof(char),size,recv_file);     // Write to the output file
        cout << "Received " << count << " of " << filesize << " bytes" << endl;
        send_packet(s,sa,buffer,FRAME_SIZE,0);          // Send acknowledgement
    }
    fclose(recv_file);
}


void put_timeout(){
    cout << "Frame has timed out" << endl;
    exit(0);
}

/**
 * PUT function
 * Performs the sending half of a request
 */
void put(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){

    char window[FRAME_SIZE * WINDOW_SIZE];  // data retention window
    char buffer[FRAME_SIZE];                // send buffer
    int filesize;
    int size = 0, sent = 0;                 // Trace variables

    FILE* send_file;


    signal(SIGINT, put_timeout);

    setTimer();

    if((send_file = fopen(filename, "rb")) != NULL){    // open the file

        // Determines the file size
        fseek(send_file, 0L, SEEK_END);
        filesize = ftell(send_file);
        fseek(send_file, 0L, SEEK_SET);

        cout << "File size: " << filesize << endl;

        strncpy(buffer, "SIZ", 3);
        memcpy(buffer + (3 * sizeof(char)), &filesize, sizeof(int)); // Add the size of the element to the buffer
        send_packet(s,sa,buffer,FRAME_SIZE,0);

        cout << "Sending..." << buffer << endl;

        memset(buffer, 0, sizeof(buffer));

        int count = offset = 0;
        int frames_outstanding = 0;
        int next = 0;

        // Start sending the file
        while (1){

            // Send as many frames as available for the given window size
            while(!feof(send_file) && frames_outstanding < WINDOW_SIZE){
                fread(buffer,1,FRAME_SIZE,send_file);                       // Read the next block of data
                memcpy(window + (offset * FRAME_SIZE), buffer, FRAME_SIZE); // Store the data in the local window
                count += send_packet(s,sa,buffer,FRAME_SIZE,0);             // Send the packet to peer
                offset = (offset + 1) % WINDOW_SIZE;                        // Update the offset
            }

            // Receive acknowledgments for at least half the frames before continuing sending 
            while(frames_outstanding > floor(WINDOW_SIZE / 2) || (feof(send_file) and frames_outstanding > 0)){
                recv_packet(s,sa,buffer,FRAME_SIZE,next);   // Receive acknowledgment from the client
                memset(buffer, 0, sizeof(buffer));          // Zero the buffer
                next = (next + 1) % WINDOW_SIZE             // Update the next frame tracker
            }

            if(feof(send_file) && !frames_outstanding) break; // Break when done reading the file and all frames are acked
        }

        fclose(send_file);
    }

}

