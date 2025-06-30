#define ESP_PORT 5000
#define ESP_IP "10.0.0.1"

#define BUFFER_SIZE 1024
#define CONTROL_BYTES 3

char buffer[BUFFER_SIZE];

#define MAC_ADDRESS_SIZE 6

#define RETURN_SUCCESS 0
#define RETURN_TERMINATE 1
#define RETURN_INPUT_ERROR 2
#define RETURN_ESP_ERROR 3
#define RETURN_APP_ERROR 4

#define NUM_OPTIONS 6
#define TERMINATE_OPT_CODE 'x'
#define STATUS_OPT_CODE 's'
#define MESSAGE_OPT_CODE 'm'
#define TIME_OPT_CODE 't'
#define DEST_OPT_CODE 'd'
#define HELP_OPT_CODE 'h'

struct OPTION{
    const char opt_code;
    const char* const help_msg;
    int (* const handler)(const int sockfd);
};

int handle_termination(const int sockfd);

int handle_status(const int sockfd);

int handle_message(const int sockfd);

int handle_time(const int sockfd);

int handle_destination(const int sockfd);

int handle_help(const int sockfd);

const struct OPTION options[NUM_OPTIONS] = {
    { TERMINATE_OPT_CODE, "terminate connection", handle_termination },
    { STATUS_OPT_CODE, "get current status", handle_status},
    { MESSAGE_OPT_CODE, "set message for the ESP to send", handle_message},
    { TIME_OPT_CODE, "set the time interval", handle_time},
    { DEST_OPT_CODE, "set destination mac address", handle_destination},
    { HELP_OPT_CODE, "print this help message", handle_help}
};

// Calculate number of elements in the array
//const int NUM_OPTIONS = sizeof(options) / sizeof(struct OPTION);

#define ESP_RESP_OK (char)0x0
#define ESP_RESP_ERROR (char)0x1