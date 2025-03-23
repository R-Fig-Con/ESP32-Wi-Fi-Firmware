
TRAFFIC_GEN::TRAFFIC_GEN(void){
    time_to_next = millis();
}


void TRAFFIC_GEN::init(bool (* sendDataF)(CCPACKET), uint8_t my_addr[6], uint8_t destination_addr[6]){
    sendData = sendDataF;
    memcpy(destination_mac, destination_addr, 6);
    memcpy(source_mac, my_addr, 6);
}

bool TRAFFIC_GEN::setTime(uint8_t time_mode, uint16_t waiting_time){
    if (time_mode != TRF_GEN_CONST && time_mode != TRF_GEN_GAUSS){
        return false;
    }
    
    time_interval_mode = time_mode;
    time_interval = waiting_time;
    return true;
}

void TRAFFIC_GEN::setMessage(uint8_t given_message[TRF_GEN_MAX_MSG_LEN], uint16_t size){
    given_message = message;
    message_size = size;
}

bool TRAFFIC_GEN::clearToSend(){
    unsigned long now = millis();

    bool result = now > time_to_next;

    if (result){
        //TODO calculate new time_to_next
    }

    return result;
    
}

void TRAFFIC_GEN::setSender(bool (* sendDataF)(CCPACKET)){
    sendData = sendDataF;
}
