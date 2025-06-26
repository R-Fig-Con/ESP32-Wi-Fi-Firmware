#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define ESP_PORT 5000
#define ESP_IP "10.0.0.1"

#define MSG_BUFFER_SIZE 1024

struct OPTION{
    const char ch_code;
    const char* help_msg;
    const int (*handler)(int sockfd);
};

int handle_termination(int sockfd);

int handle_message(int sockfd);

int handle_time(int sockfd);

int handle_destination(int sockfd);

int handle_help(int sockfd);

const struct OPTION options[] = {
    { 'x', "terminate connection", handle_termination },
    { 'm', "set message for the ESP to send", handle_message},
    { 't', "set the time interval", handle_time},
    { 'd', "set destination mac address", handle_destination},
    { 'h', "print help message", handle_help}
};

// Calculate number of elements in the array
const int NUM_OPTIONS = sizeof(options) / sizeof(struct OPTION);