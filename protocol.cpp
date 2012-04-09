#include "ftplib.cpp"

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define FRAME_SIZE  128      // Size (in bytes) of each packet
#define WINDOW_SIZE 5

#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define HEADER "%s\t%s\t%s" // Format string for headers

/**
 * GET function
 * Performs the receiving half of a request
 */
void get(SOCKET s, SOCKADDR_IN sa, char * username, char* filename){
    char buffer[FRAME_SIZE];
    int count, offset, recv, filesize, size;

    FILE* recv_file = fopen(filename, "wb");

    recv_packet(s, sa, buffer, FRAME_SIZE, 0); // Receives the filesize negotiation packet

    memcpy(&filesize, buffer + (3 * sizeof(char)), sizeof(int));

    cout << "File size: " << filesize << endl;

    offset = recv = count = 0;

    int expected_size = WINDOW_SIZE;
    int recv_count, nak;
    int next = 0;
    int packet_id;
    // Receive the file
    while(1){
        nak = -1;
        recv_count = 0;
        next = offset;
        while(count < filesize && recv_count < expected_size){
            if(filesize - count >= (FRAME_SIZE))    size = (FRAME_SIZE / sizeof(char));         // Read the full buffer
            else                                    size = ((filesize - count) / sizeof(char)); // Read a subset of the buffer
            if((packet_id = recv_packet(s,sa,buffer,FRAME_SIZE,offset)) > 0){ // Receive the packet from the peer
                count += FRAME_SIZE;
                fwrite(buffer,sizeof(char),size,recv_file);     // Write to the output file
                cout << "Received packet " << offset << "(" << count << " of " << filesize << " bytes)" << endl;
                offset = (offset + 1) % WINDOW_SIZE;            // Update the offset
            }else{
                cout << "Error in recv " << recv << endl;
                nak = offset;
                break;
            }
            recv_count++;
        }
        while(recv_count > 0 || nak >= 0){
            memset(buffer,0,FRAME_SIZE);
            if(next != nak) strncpy(buffer, "ACK", 3);  // Send ACK
            else            strncpy(buffer, "NAK", 3);  // Send NAK
            send_packet(s,sa,buffer,FRAME_SIZE,next); // Send acknowledgement
            recv_count--;
            if(next == nak){
                offset = nak;
                cout << "Sent NAK for packet " << nak << endl;
                break; 
            } // As soon as we send a NAK we can break
            next = (next + 1) % WINDOW_SIZE;
        }

        if(count >= filesize) break;
    }
    fclose(recv_file);
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

    if((send_file = fopen(filename, "rb")) != NULL){    // open the file

        // Determines the file size
        fseek(send_file, 0L, SEEK_END);
        filesize = ftell(send_file);
        fseek(send_file, 0L, SEEK_SET);

        cout << "File size: " << filesize << endl;

        strncpy(buffer, "SIZ", 3);
        memcpy(buffer + (3 * sizeof(char)), &filesize, sizeof(int)); // Add the size of the element to the buffer
        send_packet(s,sa,buffer,FRAME_SIZE,0);

        memset(buffer, 0, sizeof(buffer));

        int count = 0;
        int offset = 0;
        int frames_outstanding = 0;
        int next = 0;
        bool resend = false;
        int packet_id;

        // Start sending the file
        while (1){

            if(next != offset) resend = true;

            // Send as many frames as available for the given window size
            while(!feof(send_file) && frames_outstanding < WINDOW_SIZE){
                if(next == offset) resend = false;

                if(!resend){
                    fread(buffer,1,FRAME_SIZE,send_file);                       // Read the next block of data
                    memcpy(window + (offset * FRAME_SIZE), buffer, FRAME_SIZE); // Store the data in the local window
                    count += send_packet(s,sa,buffer,FRAME_SIZE,offset);             // Send the packet to peer
                    offset = (offset + 1) % WINDOW_SIZE;                        // Update the offset
                    cout << "Sent " << count << " bytes" << endl;
                }else{
                    // Resend by copying the data from the window
                    memcpy(buffer, window + (next * FRAME_SIZE), FRAME_SIZE);
                    send_packet(s,sa,buffer,FRAME_SIZE,next);
                    cout << "Resent packet " << next << endl;
                    next = (next + 1) % WINDOW_SIZE;
                }
                frames_outstanding++;
            }

            // Receive acknowledgments for at least half the frames before continuing sending 
            while(frames_outstanding > 0){
                cout << "Waiting for ack" << endl;
                if((packet_id = recv_packet(s,sa,buffer,FRAME_SIZE,next)) == 0){
                    cout << "Client does not seem to have received packet " << next << endl;
                    break;
                }
                // Receive acknowledgment from the client
                cout << "Got " << buffer << " from client" << endl;
                if(!strncmp(buffer,"NAK", 3)){
                    cout << "Client sent NAK " << packet_id << ", rebalancing window and resending" << endl;
    
                    break;
                }
                memset(buffer, 0, sizeof(buffer));          // Zero the buffer
                next = (next + 1) % WINDOW_SIZE;             // Update the next frame tracker
                frames_outstanding --;
            }

            frames_outstanding = 0;

            if(feof(send_file) && !frames_outstanding) break; // Break when done reading the file and all frames are acked
        }

        fclose(send_file);
    }

}

