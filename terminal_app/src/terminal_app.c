#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "terminal_app.h"
#include "util.h"

int communicate();

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

    while(1){
        printf("Enter a command: ");
        input_code = get_char_and_flush();

        for (i = 0; i < NUM_OPTIONS; ++i) {

            if(options[i].ch_code != input_code){
                continue;
            }
            
            status = options[i].handler(sockfd);
            switch(status){
                case RETURN_TERMINATE:
                    return RETURN_SUCCESS;
                case RETURN_ESP_ERROR:
                    printf("ESP error: %s\n", buffer+1); //First byte is status
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

int handle_help(const int sockfd){

    for (int i = 0; i < NUM_OPTIONS; ++i) {
        printf("\t'%c': %s\n", options[i].ch_code, options[i].help_msg);
    }

    return 0;
}

int handle_message(const int sockfd){
    // TODO(Implement, this was only for testing)

    // Send a message
    const char *message = "Hello from the app!";
    send(sockfd, message, strlen(message), 0);
    printf("Message sent to ESP32.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    buffer[bytes_read] = '\0';
    printf("Response from ESP32: %s\n", buffer);

    // Send again
    const char *message_2 = "Hello again!";
    send(sockfd, message_2, strlen(message_2), 0);
    printf("Message sent to ESP32.\n");

    bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    buffer[bytes_read] = '\0';
    printf("Response from ESP32: %s\n", buffer);

    return RETURN_SUCCESS;
}

int handle_termination(const int sockfd){

    return RETURN_TERMINATE;
}

int handle_status(const int sockfd){

    //TODO(Request current config - interval type, time interval, message, destination address - from ESP)
    printf("TODO(Request current config - interval type, time interval, message, destination address - from ESP)\n");

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
            return RETURN_APP_ERROR;
        }

        number = get_number_from_str(buffer, BUFFER_SIZE);

        if(number.status == IS_NUMBER){
            break;
        }
        printf("\t- Please introduce a valid number.\n");
    }

    // Send command 2 bytes, + 2 bytes of value
    buffer[0] = 't'; //0 - Set Time Command
    buffer[1] = input_code; //1 - Type of time
    fit_number_w_left_padding(buffer+2, 16, number.value); //2 & 3 - Time interval value
    buffer[4] = '\0'; //4

    // Send message
    send(sockfd, buffer, 4, 0);
    printf("Time config sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    buffer[bytes_read] = '\0';
    if(buffer[0] != ESP_RESP_OK){
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}

int handle_destination(const int sockfd){

    //TODO(Set destination address for ESP)
    printf("TODO(Set destination address for ESP)\n");

    return 0;
}