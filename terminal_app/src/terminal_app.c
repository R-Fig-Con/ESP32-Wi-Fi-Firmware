#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "terminal_app.h"
#include "util.h"

int communicate(const int sockfd);

int main() {
    struct sockaddr_in addr;
    int sockfd;

    // Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Could not create socket.");
        exit(EXIT_FAILURE);
    }

    // Set the socket address structure
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ESP_PORT);

    int error = inet_pton(AF_INET, ESP_IP, &addr.sin_addr);
    if (error == 0) {
        perror("Invalid network address.");
        exit(EXIT_FAILURE);
    } else if (error == -1){
        perror("Invalid address family.");
        exit(EXIT_FAILURE);
    }

    printf("Connecting...\n");

    //Connect to ESP
    error = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if ( error == -1) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to ESP32 node.\n");

    error = communicate(sockfd);
    
    // Close socket
    close(sockfd);
    printf("Connection terminated.\n");

    if(error != RETURN_SUCCESS){
        perror("Error communicating.");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int communicate(const int sockfd){
    char input_code;
    int status, i;

    printf("Options:\n");
    handle_help(sockfd);

    while(1){
        printf("Enter a command: ");
        input_code = get_char_and_flush();

        for (i = 0; i < NUM_OPTIONS; ++i) {

            if(options[i].opt_code != input_code){
                continue;
            }
            
            status = options[i].handler(sockfd);
            switch(status){
                case RETURN_TERMINATE:
                    return RETURN_SUCCESS;
                case RETURN_ESP_ERROR:
                    printf("ESP error: %s\n", buffer+1); //First byte of response is status
                    break;
                case RETURN_INPUT_ERROR:
                    printf("Error proccessing input.");
                    break;
                case RETURN_SUCCESS:
                default:
                    printf("Operation concluded successfully\n");
                    break;
            }

            break;
        }

        if(i == NUM_OPTIONS){
            printf("Invalid command\n");
        }
    }

    return RETURN_APP_ERROR;
}

uint16_t set_control_bytes(char* buffer, uint16_t len, char opt_code){
    
    len += CONTROL_BYTES;
    int offset = 0;
    buffer[offset++] = len & 0xFF;
    buffer[offset++] = len>>8; //ESP is little endian, so the most significant bits come last
    buffer[offset++] = opt_code; // Command

    return len;// + CONTROL_BYTES;
}

int handle_help(const int sockfd){
    (void)sockfd; //Remove compile warning

    for (int i = 0; i < NUM_OPTIONS; ++i) {
        printf("\t'%c': %s\n", options[i].opt_code, options[i].help_msg);
    }

    return 0;
}

int handle_message(const int sockfd){
    
    printf("Message for ESP to send: ");
    if (fgets(buffer + CONTROL_BYTES, BUFFER_SIZE-CONTROL_BYTES, stdin) == NULL) {
        // Error or EOF
        return RETURN_INPUT_ERROR;
    }

    // Just "strlen(buffer)" is undefined behaviour because the first bytes have not been set and might be '\0'
    uint16_t len = strlen(buffer + CONTROL_BYTES);
    len = set_control_bytes(buffer, len, MESSAGE_OPT_CODE);
    if(buffer[len-1]=='\n') buffer[len-1]='\0';

    //For DEBUG
    printf("Len: %d\nNew Message: %s\n", len, buffer + CONTROL_BYTES);
    //---------

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Message config sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(buffer[0] != ESP_RESP_OK){
        buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}

int handle_termination(const int sockfd){
    (void)sockfd; //Remove compile warning

    return RETURN_TERMINATE;
}

int handle_status(const int sockfd){

    uint16_t len = set_control_bytes(buffer, 0, STATUS_OPT_CODE);

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Time config sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(buffer[0] != ESP_RESP_OK){
        buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    int offset = 1; //Already read the response status byte

    char type = buffer[offset++];
    uint16_t time = buffer[offset++];
    time += (buffer[offset++]<<8);

    uint16_t success_count = buffer[offset++];
    success_count += buffer[offset++] << 8;

    uint16_t failure_count = buffer[offset++];
    failure_count += buffer[offset++] << 8;

    uint32_t retry_count = buffer[offset++];
    retry_count += buffer[offset++] << 8;
    retry_count += buffer[offset++] << 16;
    retry_count += buffer[offset++] << 24;

    uint32_t running_time = buffer[offset++];
    running_time += buffer[offset++] << 8;
    running_time += buffer[offset++] << 16;
    running_time += buffer[offset++] << 24;

    unsigned char dest_mac[MAC_ADDRESS_SIZE];
    memcpy(dest_mac, buffer + offset, MAC_ADDRESS_SIZE);

    char* msg = &buffer[offset+MAC_ADDRESS_SIZE];

    printf("Time (ms): %u | Interval type: %c\n"
            "Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X\n"
            "Message: %s\n",
            time, type,
            dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5],
            msg);

    printf("Mac protocol; running time: %ul\n"
            "Success count: %u\n"
            "Failure count: %u\n"
            "Retry count: %ul\n",
            running_time,
            success_count,
            failure_count,
            retry_count);

    return RETURN_SUCCESS;
}

int handle_time(const int sockfd){

    char input_code;
    while(1){
        printf("Set (c)onstant interval or (g)aussian interval: ");
        input_code = get_char_and_flush();

        if(input_code == 'g' || input_code == 'c'){
            break;
        }
        printf("\t- Invalid option.\n");
    }

    struct NUMBER number;
    while(1){
        printf("Time (milliseconds): ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            // Error or EOF
            return RETURN_INPUT_ERROR;
        }

        number = get_number_from_str(buffer, BUFFER_SIZE);

        if(number.status == IS_NUMBER){
            break;
        }
        printf("\t- Please introduce a valid number.\n");
    }

    uint16_t len = sizeof(input_code) + sizeof(number.value); //The sub-command byte + bytes of time
    len = set_control_bytes(buffer, len, TIME_OPT_CODE);

    buffer[CONTROL_BYTES] = input_code; // Type of interval
    buffer[CONTROL_BYTES+1] = number.value >> 8;
    buffer[CONTROL_BYTES+2] = number.value & 0xFF;

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Time config sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(buffer[0] != ESP_RESP_OK){
        buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}

int handle_destination(const int sockfd){

    char mac_addr[MAC_ADDRESS_SIZE];
    while(1){
        printf("Destination MAC Address: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            // Error or EOF
            return RETURN_INPUT_ERROR;
        }

        int status = get_mac_from_str(buffer, BUFFER_SIZE, mac_addr, MAC_ADDRESS_SIZE);

        if(status == MAC_IS_VALID){
            break;
        }
        printf("\t- Please introduce a valid address.\n");
    }

    uint16_t len = MAC_ADDRESS_SIZE;
    len = set_control_bytes(buffer, len, DEST_OPT_CODE);
    memcpy(buffer+CONTROL_BYTES, mac_addr, MAC_ADDRESS_SIZE);

    /*printf("New MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]); //DEBUG */

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Destination config sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(buffer[0] != ESP_RESP_OK){
        buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}

int handle_backoff(const int sockfd){

    char input_code;
    while(1){
        printf("Set (m)ild backoff; (l)inear; or (n)o backoff: ");
        input_code = get_char_and_flush();

        if(input_code == 'm' || input_code == 'l' || input_code == 'n'){
            break;
        }
        printf("\t- Invalid option.\n");
    }

    uint16_t len = (uint16_t) sizeof(char);

    len = set_control_bytes(buffer, len, BACKOFF_PROTOCOL_OPT_CODE);
    
    buffer[CONTROL_BYTES] = input_code;

    // Send message
    send(sockfd, buffer, len, 0);
    printf("Backoff protocol config sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(buffer[0] != ESP_RESP_OK){
        buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}