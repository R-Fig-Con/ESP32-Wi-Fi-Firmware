#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "communication.h"



#define MAX_MESSAGE_SIZE 1024

#define CONTROL_BYTES_SIZE 3

static char buffer[MAX_MESSAGE_SIZE];

static int sockfd;

static uint16_t set_control_bytes(uint16_t len, char opt_code){
    
    len += CONTROL_BYTES_SIZE;
    int offset = 0;
    buffer[offset++] = len & 0xFF;
    buffer[offset++] = len>>8; //ESP is little endian, so the most significant bits come last
    buffer[offset++] = opt_code; // Command

    return len;
}

static int receive_response(){
    // Receive response
    int bytes_read = read(sockfd, buffer, MAX_MESSAGE_SIZE - 1);
    if(buffer[0] != RETURN_SUCCESS){
        buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}


int connection_start(){
    struct sockaddr_in addr;

    // Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Could not create socket.");
        return 0;
    }

    // Set the socket address structure
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ESP_PORT);

    int error = inet_pton(AF_INET, ESP_IP, &addr.sin_addr);
    if (error == 0) {
        perror("Invalid network address.");
        return 0;
    } else if (error == -1){
        perror("Invalid address family.");
        return 0;
    }

    printf("Connecting...\n");

    //Connect to ESP
    error = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if ( error == -1) {
        perror("Connection Failed");
        return 0;
    }

    return 1;
}


int connection_end(){
    close(sockfd);
}


int set_time(time_option time_type, uint16_t number){

    char input_code = (GAUSSIAN == time_type) ? 'g' : 'c';

    uint16_t len = sizeof(input_code) + sizeof(number); //The sub-command byte + bytes of time
    len = set_control_bytes(len, TIME_OPT_CODE);

    buffer[CONTROL_BYTES_SIZE] = input_code; // Type of interval
    buffer[CONTROL_BYTES_SIZE + 1] = number >> 8;
    buffer[CONTROL_BYTES_SIZE + 2] = number & 0xFF;

    return receive_response();
}

int set_destination(char address[MAC_ADDRESS_SIZE]){
    uint16_t len = MAC_ADDRESS_SIZE;
    len = set_control_bytes(len, DEST_OPT_CODE);
    memcpy(buffer + CONTROL_BYTES_SIZE, address, MAC_ADDRESS_SIZE);

    /*printf("New MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]); //DEBUG */

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Destination config sent to ESP node.\n");

    return receive_response();
}

static int set_message(char option, char* data, uint16_t length){
    // Just "strlen(buffer)" is undefined behaviour because the first bytes have not been set and might be '\0'
    uint16_t len = strlen(buffer + CONTROL_BYTES_SIZE);
    len = set_control_bytes(len, MESSAGE_OPT_CODE);

    memcpy(buffer + CONTROL_BYTES_SIZE, data, length);

    if(buffer[len-1]=='\n') buffer[len-1]='\0';

    //For DEBUG
    printf("Len: %d\nNew Message: %s\n", len, buffer + CONTROL_BYTES_SIZE);
    //---------

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Message config sent to ESP node.\n");

    return receive_response();
}

static int set_backoff(char algorithm){
    uint16_t len = (uint16_t) sizeof(char);

    len = set_control_bytes(len, BACKOFF_PROTOCOL_OPT_CODE);
    
    buffer[CONTROL_BYTES_SIZE] = algorithm;

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Backoff protocol config sent to ESP node.\n");

    return receive_response();
}