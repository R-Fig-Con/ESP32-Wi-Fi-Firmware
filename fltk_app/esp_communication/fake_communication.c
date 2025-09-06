#include "communication.h"

int connection_start(const char *ip_address){
    return RETURN_SUCCESS;
}

void connection_end(const char *ip_address){}

int set_time(const char *ip_address, time_option time_type, uint16_t number){
    return RETURN_SUCCESS;
}

int set_destination(const char *ip_address, char address[MAC_ADDRESS_SIZE]){
    return RETURN_SUCCESS;
}

int set_message(const char *ip_address, char *data, uint16_t length){
    return RETURN_SUCCESS;
}

int set_backoff(const char *ip_address, backoff_option option){
    return RETURN_SUCCESS;
}

int get_status(const char *ip_address, status *mem){
    return RETURN_SUCCESS;
}
