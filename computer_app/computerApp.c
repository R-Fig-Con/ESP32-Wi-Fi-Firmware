#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.1.100"  // ESP32 IP address, corresponding to local static IP given to ESP 
#define SERVER_PORT 1234        // Port used by ESP32

/**
 * Communicates with esp, assuming the port and IP being used on the defines
 */

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024] = {0};

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Connect to ESP32
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to ESP32 failed");
        return 1;
    }

    printf("Connected to ESP32\n");

    // Send message
    const char *message = "Hello from PC!\n";
    send(sock, message, strlen(message), 0);

    // Receive response
    read(sock, buffer, sizeof(buffer));
    printf("ESP32 says: %s\n", buffer);

    close(sock);
    return 0;
}
