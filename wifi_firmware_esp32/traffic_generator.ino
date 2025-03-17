#include "src/traffic_generator/traffic_generator.h"

TRAFFIC_GEN::TRAFFIC_GEN(void){
    time_to_next = milis();
}


/**
 * setTime
 *
 * Set the interval mode and time to custom values.
 *
 * 'time_mode' Will set the time inverval mode of the time between packets.
 * 
 * 'waiting_time' The constant time if mode is TRF_GEN_CONST, or the median time if mode is TRF_GEN_GAUSS
 *
 * returns false if time_mode is invalid; else true
 */
bool TRAFFIC_GEN::setTime(uint8_t time_mode, uint16_t waiting_time){
    if (time_mode != TRF_GEN_CONST && time_mode != TRF_GEN_GAUSS){
        return false;
    }
    
    time_interval_mode = time_mode;
    time_interval = waiting_time;
    return true;
}


/**
 * Sets message to be sent in each frame.
 */
void TRAFFIC_GEN::setMessage(uint8_t given_message[TRF_GEN_MAX_MSG_LEN], uint16_t size){
    given_message = message;
    message_size = size;
}

/**
 * clearToSend
 *
 * Checks if enough time has passed to be able to send
 * 
 * returns true if it can be sent
 */
bool TRAFFIC_GEN::clearToSend(){
    unsigned long now = milis();

    bool result = now > time_to_next;

    if (result){
        /* TODO calculate new time_to_next  */
    }

    return result;
    
}


/**
 * set_sender
 *
 * Sets the function that will be used to send the packet
 */
void TRAFFIC_GEN::setSender(bool (* sendDataF)(CCPACKET)){
    sendData = sendDataF;
}