#include "terminal_app.h"

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

    if(error != 0){
        perror("Error communicating.");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int communicate(int sockfd){
    char input_code, flush;

    while(1){
        printf("Enter a command: ");
        input_code = getchar();
        while ((flush = getchar()) != '\n'); // Clear input buffer

        for (int i = 0; i < NUM_OPTIONS; ++i) {
            if(options[i].ch_code == input_code){
                int result = options[i].handler(sockfd);
                //TODO: Proccess result
            }
        }
    }

    return 0;
}

int handle_help(int sockfd){

    for (int i = 0; i < NUM_OPTIONS; ++i) {
        printf("\t'%c': %s\n", options[i].ch_code, options[i].help_msg);
    }

    return 0;
}

int handle_message(int sockfd){
    // TODO(Implement, this was only for testing)
    char buffer[MSG_BUFFER_SIZE];

    // Send a message
    const char *message = "Hello from the app!";
    send(sockfd, message, strlen(message), 0);
    printf("Message sent to ESP32.\n");

    // Receive response
    int bytes_read = read(sockfd, buffer, MSG_BUFFER_SIZE - 1);
    buffer[bytes_read] = '\0';
    printf("Response from ESP32: %s\n", buffer);

    // Send again
    const char *message_2 = "Hello again!";
    send(sockfd, message_2, strlen(message_2), 0);
    printf("Message sent to ESP32.\n");

    bytes_read = read(sockfd, buffer, MSG_BUFFER_SIZE - 1);
    buffer[bytes_read] = '\0';
    printf("Response from ESP32: %s\n", buffer);

    return 0;
}

int handle_termination(int sockfd){

    //TODO(Return code - pre-defined in '.h' - that "communicate()" will interpret as an order to exit. The socket is closed in main)

    return 0;
}

int handle_time(int sockfd){

    //TODO

    return 0;
}

int handle_destination(int sockfd){

    //TODO

    return 0;
}