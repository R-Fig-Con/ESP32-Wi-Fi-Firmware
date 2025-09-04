#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <string.h>

#include "communication.h"

#define ESP_PORT 5000

#define TERMINATE_OPT_CODE 'x'
#define STATUS_OPT_CODE 's'
#define MESSAGE_OPT_CODE 'm'
#define TIME_OPT_CODE 't'
#define DEST_OPT_CODE 'd'
#define BACKOFF_PROTOCOL_OPT_CODE 'b'

#define ESP_RESP_OK (char)0x0
#define ESP_RESP_ERROR (char)0x1

#define CONTROL_BYTES_SIZE 3

/** list of nodes with key address connected to value sockfd */
typedef struct node
{
    const char *address;
    int sockfd;
    struct node *next;
} pair_node;

static pair_node *head = NULL;

static void add_node(const char *key, int value)
{
    pair_node *current = head;
    if (head == NULL)
    {
        current = (pair_node *)malloc(sizeof(pair_node));
        current->address = key;
        current->sockfd = value;
        head = current;
        return;
    }

    pair_node *next = head->next;

    while (1)
    {
        if (next == NULL)
        {
            next = (pair_node *)malloc(sizeof(pair_node));
            next->address = key;
            next->sockfd = value;
            current->next = next;
            return;
        }

        current = current->next;
        next = next->next;
    }
}

static int get_sock_fd(const char *key)
{
    pair_node *current = head;

    while (1)
    {
        if (strcmp(current->address, key))
        {
            return current->sockfd;
        }
        current = current->next;
    }
}

static void remove_node(const char *key)
{
    // assumes it does not remove when it does not exist
    pair_node *prev = head;

    pair_node *current = head->next;

    while (1)
    {
        if (strcmp(current->address, key))
        {
            prev->next = current->next;
            free(current);
            return;
        }

        current = current->next;
        prev = prev->next;
    }
}

// if instance discovery is multi-threaded itself this is not safe
char communication_buffer[MAX_MESSAGE_SIZE];

static uint16_t set_control_bytes(uint16_t len, char opt_code)
{

    len += CONTROL_BYTES_SIZE;
    int offset = 0;
    communication_buffer[offset++] = len & 0xFF;
    communication_buffer[offset++] = len >> 8; // ESP is little endian, so the most significant bits come last
    communication_buffer[offset++] = opt_code; // Command

    return len;
}

inline static int receive_response(int sockfd)
{
    // Receive response
    int bytes_read = read(sockfd, communication_buffer, MAX_MESSAGE_SIZE - 1);
    if (communication_buffer[0] == ESP_RESP_ERROR)
    {
        communication_buffer[bytes_read] = '\0';
        printf("Error of length %d, first char %c; received from esp: %s\n", bytes_read, communication_buffer[0], communication_buffer);
        return RETURN_ESP_ERROR;
    }

    return RETURN_SUCCESS;
}

int connection_start(const char *ip_address)
{
    struct sockaddr_in addr;

    // Create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Could not create socket.");
        return RETURN_ESP_ERROR;
    }

    // Set the socket address structure
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ESP_PORT);

    int error = inet_pton(AF_INET, ip_address, &addr.sin_addr);
    if (error == 0)
    {
        perror("Invalid network address.");
        return RETURN_ESP_ERROR;
    }
    else if (error == -1)
    {
        perror("Invalid address family.");
        return RETURN_ESP_ERROR;
    }

    printf("Connecting...\n");

    // Connect to ESP
    error = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (error == -1)
    {
        perror("Connection Failed");
        return RETURN_ESP_ERROR;
    }

    add_node(ip_address, sockfd);

    return RETURN_SUCCESS;
}

void connection_end(const char *ip_address)
{
    close(get_sock_fd(ip_address));
    remove_node(ip_address);
}

int set_time(const char *ip_address, time_option time_type, uint16_t number)
{

    int sockfd = get_sock_fd(ip_address);

    char input_code = (GAUSSIAN == time_type) ? 'g' : 'c';

    uint16_t len = sizeof(input_code) + sizeof(number); // The sub-command byte + bytes of time
    len = set_control_bytes(len, TIME_OPT_CODE);

    communication_buffer[CONTROL_BYTES_SIZE] = input_code; // Type of interval
    communication_buffer[CONTROL_BYTES_SIZE + 1] = number >> 8;
    communication_buffer[CONTROL_BYTES_SIZE + 2] = number & 0xFF;

    send(sockfd, communication_buffer, len, 0);
    printf("Time config sent to ESP node.\n");

    return receive_response(sockfd);
}

int set_destination(const char *ip_address, char address[MAC_ADDRESS_SIZE])
{
    int sockfd = get_sock_fd(ip_address);

    uint16_t len = MAC_ADDRESS_SIZE;
    len = set_control_bytes(len, DEST_OPT_CODE);
    memcpy(communication_buffer + CONTROL_BYTES_SIZE, address, MAC_ADDRESS_SIZE);

    /*printf("New MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]); //DEBUG */

    // Send message
    send(sockfd, communication_buffer, len, 0);
    printf("Destination config sent to ESP node.\n");

    return receive_response(sockfd);
}

int set_message(const char *ip_address, char *data, uint16_t length)
{
    int sockfd = get_sock_fd(ip_address);
    printf("set message; length: %d\n", length);

    uint16_t len = length;
    len = set_control_bytes(len, MESSAGE_OPT_CODE);

    memcpy(communication_buffer + CONTROL_BYTES_SIZE, data, length);

    if (communication_buffer[len - 1] == '\n')
        communication_buffer[len - 1] = '\0';

    // For DEBUG
    printf("Len: %d\nNew Message: %s\n", len, communication_buffer + CONTROL_BYTES_SIZE);
    //---------

    // Send message
    send(sockfd, communication_buffer, len, 0);
    printf("Message config sent to ESP node.\n");

    return receive_response(sockfd);
}

int set_backoff(const char *ip_address, backoff_option option)
{
    int sockfd = get_sock_fd(ip_address);
    uint16_t len = (uint16_t)sizeof(char);

    len = set_control_bytes(len, BACKOFF_PROTOCOL_OPT_CODE);

    char backoff_option;

    printf("Chosen backoff %d\n", option);

    switch (option)
    {
    case NONE:
        backoff_option = 'n';
        break;

    case LINEAR:
        backoff_option = 'l';
        break;

    case MILD:
        backoff_option = 'm';
        break;

    default:
        break;
    }

    communication_buffer[CONTROL_BYTES_SIZE] = backoff_option;

    printf("Buffer print: %c; %c; %c; %c\n",
           communication_buffer[0],
           communication_buffer[1],
           communication_buffer[2],
           communication_buffer[3]);

    // Send message
    send(sockfd, communication_buffer, len, 0);
    printf("Backoff protocol config sent to ESP node.\n");

    return receive_response(sockfd);
}

int get_status(const char *ip_address, status *mem)
{
    int sockfd = get_sock_fd(ip_address);
    uint16_t len = set_control_bytes(0, STATUS_OPT_CODE);

    // Send message
    send(sockfd, communication_buffer, len, 0);
    printf("Status request sent to ESP node.\n");

    // Receive response
    int bytes_read = read(sockfd, communication_buffer, MAX_MESSAGE_SIZE - 1);
    if (communication_buffer[0] != RETURN_SUCCESS)
    {
        communication_buffer[bytes_read] = '\0';
        return RETURN_ESP_ERROR;
    }

    int offset = 1; // Already read the response status byte

    mem->type = communication_buffer[offset++];

    mem->time = (uint8_t)communication_buffer[offset++];
    mem->time += ((uint8_t)communication_buffer[offset++]) << 8;

    mem->success_count = (uint8_t)communication_buffer[offset++];
    mem->success_count += ((uint8_t)communication_buffer[offset++]) << 8;

    mem->failure_count = (uint8_t)communication_buffer[offset++];
    mem->failure_count += ((uint8_t)communication_buffer[offset++]) << 8;

    mem->retry_count = (uint8_t)communication_buffer[offset++];
    mem->retry_count += ((uint8_t)communication_buffer[offset++]) << 8;
    mem->retry_count += ((uint8_t)communication_buffer[offset++]) << 16;
    mem->retry_count += ((uint8_t)communication_buffer[offset++]) << 24;

    mem->running_time = (uint8_t)communication_buffer[offset++];
    mem->running_time += ((uint8_t)communication_buffer[offset++]) << 8;
    mem->running_time += ((uint8_t)communication_buffer[offset++]) << 16;
    mem->running_time += ((uint8_t)communication_buffer[offset++]) << 24;

    memcpy(mem->dest_mac, communication_buffer + offset, MAC_ADDRESS_SIZE);

    int msg_size = bytes_read - offset;
    char *msg = (char *)malloc((size_t)msg_size);

    memcpy(mem->msg, communication_buffer + offset + MAC_ADDRESS_SIZE, msg_size);

    mem->msg = msg;

    return RETURN_SUCCESS;
}
