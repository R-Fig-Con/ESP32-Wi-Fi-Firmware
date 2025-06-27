#define ESP_PORT 5000
#define ESP_IP "10.0.0.1"

#define BUFFER_SIZE 1024

#define RETURN_SUCCESS 0
#define RETURN_TERMINATE 1
#define RETURN_APP_ERROR 2
#define RETURN_ESP_ERROR 3

#define ESP_RESP_OK (char)0x0
#define ESP_RESP_ERROR (char)0x1

char buffer[BUFFER_SIZE];

struct OPTION{
    const char ch_code;
    const char* const help_msg;
    int (* const handler)(const int sockfd);
};

int handle_termination(const int sockfd);

int handle_status(const int sockfd);

int handle_message(const int sockfd);

int handle_time(const int sockfd);

int handle_destination(const int sockfd);

int handle_help(const int sockfd);

const struct OPTION options[] = {
    { 'x', "terminate connection", handle_termination },
    { 's', "get current status", handle_status},
    { 'm', "set message for the ESP to send", handle_message},
    { 't', "set the time interval", handle_time},
    { 'd', "set destination mac address", handle_destination},
    { 'h', "print help message", handle_help}
};

// Calculate number of elements in the array
const int NUM_OPTIONS = sizeof(options) / sizeof(struct OPTION);