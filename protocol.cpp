#include "ftplib.cpp"

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define FRAME_SIZE  128     // Size (in bytes) of each packet
#define WINDOW_SIZE 19

#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define HEADER "%s\t%s\t%s" // Format string for headers

/**
 * GET function
 * Performs the receiving half of a request
 */
void get(SOCKET s, SOCKADDR_IN sa, char * username, char* filename, int client_num, int server_num, FILE* logfile){
    char buffer[FRAME_SIZE];
    int count, offset, recv, filesize, size;
    char tracebuf[128];

    FILE* recv_file = fopen(filename, "wb");

    if(recv_safe(s, sa, buffer, FRAME_SIZE, 101) == 101){ // Receives the filesize negotiation packet

        memcpy(&filesize, buffer + (3 * sizeof(char)), sizeof(int));

        cout << "Got filesize " << filesize << " starting transfer..." << endl;

        sprintf(tracebuf, "Filesize %d", filesize);
        write_log(logfile, username, tracebuf);

        offset = recv = count = 0;

        int expected_size = WINDOW_SIZE + 1;
        int recv_count, nak;
        int next = 0;
        int packet_id;
        // Receive the file
        while(1){
            nak = -1;
            recv_count = 0;
            next = offset;
            while(count < filesize && recv_count < WINDOW_SIZE){
                if(filesize - count >= (FRAME_SIZE))    size = (FRAME_SIZE / sizeof(char));         // Read the full buffer
                else                                    size = ((filesize - count) / sizeof(char)); // Read a subset of the buffer
                if((packet_id = recv_packet(s,sa,buffer,FRAME_SIZE,offset)) == offset){ // Receive the packet from the peer
                    count += FRAME_SIZE;
                    fwrite(buffer,sizeof(char),size,recv_file);     // Write to the output file
                    
                    sprintf(tracebuf, "Recv %d (%d of %d)", offset, count, filesize);
                    write_log(logfile, username, tracebuf);

                    offset = (offset + 1) % expected_size;    
                    recv_count++;
                }else if(packet_id < 0){
                    cout << "Error in recv " << recv << endl;
                    nak = offset;
                    break;
                }else if(packet_id == 101){
                    fclose(recv_file);
                    return get(s, sa, username, filename, client_num, server_num, logfile);
                }
            }
            while(recv_count > 0 || nak >= 0){
                memset(buffer,0,FRAME_SIZE);
                if(next != nak) strncpy(buffer, "ACK", 3);  // Send ACK
                else            strncpy(buffer, "NAK", 3);  // Send NAK
                send_packet(s,sa,buffer,FRAME_SIZE,next); // Send acknowledgement
                recv_count--;
                if(next == nak){
                    offset = nak;
                    sprintf(tracebuf, "Sent NAK for %d", nak);
                    write_log(logfile, username, tracebuf);
                    break; 
                } // As soon as we send a NAK we can break
                next = (next + 1) % expected_size;
            }

            if(count >= filesize) break;
        }
        strncpy(buffer, "ALL", 3);
        send_packet(s, sa, buffer, FRAME_SIZE, next);
        cout << "Transfer completed! " << count << " bytes received" << endl;
        fclose(recv_file);
    }else{
        fclose(recv_file);
        return get(s, sa, username, filename, client_num, server_num, logfile);
    }
}

/**
 * PUT function
 * Performs the sending half of a request
 */
void put(SOCKET s, SOCKADDR_IN sa, char * username, char* filename, int client_num, int server_num, FILE* logfile){

    char window[FRAME_SIZE * WINDOW_SIZE];  // data retention window
    char buffer[FRAME_SIZE];                // send buffer
    int filesize;
    int size = 0, sent = 0;                 // Trace variables
    char tracebuf[128];

    FILE* send_file;

    if((send_file = fopen(filename, "rb")) != NULL){    // open the file

        // Determines the file size
        fseek(send_file, 0L, SEEK_END);
        filesize = ftell(send_file);
        fseek(send_file, 0L, SEEK_SET);

        sprintf(tracebuf, "Filesize %d", filesize);
        write_log(logfile, username, tracebuf);

        strncpy(buffer, "SIZ", 3);
        memcpy(buffer + (3 * sizeof(char)), &filesize, sizeof(int)); // Add the size of the element to the buffer
        if(send_safe(s,sa,buffer,FRAME_SIZE,101) == 101){

            cout << "Sent filesize, starting transfer..." << endl;

            memset(buffer, 0, sizeof(buffer));

            int count = 0;
            int offset = 0;
            int frames_outstanding = 0;
            int next = 0;
            bool resend = false;
            int packet_id;
            int pid_max = WINDOW_SIZE + 1;

            // Start sending the file
            while (1){
                // If the acks mismatch with the current send offset, has to be a resend
                if(next != offset && frames_outstanding > 0) resend = true;

                // Send as many frames as available for the given window size
                while((!feof(send_file) && frames_outstanding < WINDOW_SIZE) || resend){
                    if(next == offset) resend = false;

                    if(!resend){
                        if(feof(send_file)) break;
                        fread(buffer,1,FRAME_SIZE,send_file);                       // Read the next block of data
                        memcpy(window + (offset * FRAME_SIZE), buffer, FRAME_SIZE); // Store the data in the local window
                        send_packet(s,sa,buffer,FRAME_SIZE,offset);             // Send the packet to peer
                        offset = (offset + 1) % pid_max;                        // Update the offset
                        frames_outstanding++;
                    }else{
                        // Resend by copying the data from the window
                        memcpy(buffer, window + (next * FRAME_SIZE), FRAME_SIZE);
                        send_packet(s,sa,buffer,FRAME_SIZE,next);
                        sprintf(tracebuf, "Resending packet %d", next);
                        write_log(logfile, username, tracebuf);
                        next = (next + 1) % pid_max;
                    }
                }

                // Receive ACKs before continuing sending 
                while(frames_outstanding > 0){
                    if((packet_id = recv_packet(s,sa,buffer,FRAME_SIZE,next)) < 0){
                        if(count < filesize) resend = true;
                        //else frames_outstanding --;
                        break;
                    }
                    // Receive acknowledgment from the client
                    if(!strncmp(buffer,"NAK", 3)){
                        if(packet_id >= 0) next = packet_id;    // Set the next packet id to send
                        break;
                    }else if(!strncmp(buffer,"ALL", 3)){
                        frames_outstanding = 0;
                        break;
                    }
                    count += FRAME_SIZE;                    // Increment the counter
                    sprintf(tracebuf, "Sent %d bytes", count);
                    write_log(logfile, username, tracebuf);
                    memset(buffer, 0, sizeof(buffer));      // Zero the buffer
                    next = (next + 1) % pid_max;            // Update the next frame tracker
                    frames_outstanding --;                  // Another frame has been acked
                }

                if(feof(send_file) && frames_outstanding == 0) break; // Break when done reading the file and all frames are acked
            }
            cout << "File transfer completed" << endl;
            fclose(send_file);
        }else{
            fclose(send_file);
            return put(s,sa,username,filename, client_num, server_num, logfile);
        }
    }

}

