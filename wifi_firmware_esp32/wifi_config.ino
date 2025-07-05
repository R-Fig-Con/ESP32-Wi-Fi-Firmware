void wifi_com_task(void* parameter) {
    uint8_t* mac_addr = (uint8_t*) parameter;

    WiFiServer server(AP_PORT);
    WIFI_CONFIG_RET status = wifi_com_start(&server, mac_addr);

    if (status.success) {
        Serial.println("WiFi COM started successfully.");
        wifi_com_handle_con(status.server);
    } else {
        Serial.println("WiFi COM failed to start.");
    }

    // Delete the task
    vTaskDelete(NULL);
}


static struct{
    /**
     * needs to be set the same as default by hand, since identifier is enum and default is class instance creation
     */
    BACKOFF_PROTOCOLS protocol = NON_EXISTANT;

    /**
     * assuming null means no specific message given
     * 
     * substituting value will probably entail freeing earlier and doing malloc for new
    */
    char* message = NULL;

    uint16_t content_length = DEFAULT_FRAME_CONTENT_SIZE;

    //sel mac probably to add in status, either by resquest function or saving it like here
    //not used for now
    uint8_t source_mac_address[MAC_ADDRESS_SIZE];

    uint8_t destination_mac_address[MAC_ADDRESS_SIZE] = DEFAULT_MAC_ADDRESS;

    uint8_t time_mode = DEFAULT_TIME_INTERVAL_MODE;

    uint16_t waiting_time = DEFAULT_TIME_INTERVAL;

} status;

WIFI_CONFIG_RET wifi_com_start(WiFiServer* server, uint8_t my_mac[MAC_ADDRESS_SIZE]){
    
    IPAddress local_IP(AP_IP_ARR[0], AP_IP_ARR[1], AP_IP_ARR[2], AP_IP_ARR[3]);
    IPAddress gateway(AP_IP_ARR[0], AP_IP_ARR[1], AP_IP_ARR[2], AP_IP_ARR[3]);
    IPAddress subnet(255, 255, 255, 0);

    WIFI_CONFIG_RET ret = {server, false};

    char ssid[6 + MAC_STR_LEN + 1] = "ESP32-"; // Base(6) + MAC + null terminator
    sprintf(ssid + 6, "%02X:%02X:%02X:%02X:%02X:%02X", 
        my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

    const char* password = WIFI_PASSWORD;

    Serial.printf("SSID: %s\n", ssid);

    WiFi.mode(WIFI_MODE_AP);

    // Configure static IP for the SoftAP interface
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("WiFi Config Failed!");
        return ret;
    }

    // Start the SoftAP with SSID and password
    if (!WiFi.softAP(ssid, password)) {
        Serial.println("WiFi Start Failed!");
        return ret;
    }

    server->begin();

    Serial.printf("Listening on %s:%d\n", WiFi.softAPIP().toString().c_str(), AP_PORT);

    ret.success = true;
    return ret;
}

void wifi_handle_status(WiFiClient* client, uint8_t* buffer, uint16_t len){

    (void)buffer;
    (void)len;

    char type = (status.time_mode == TRF_GEN_CONST) ? 'c': 'g';

    const int size = 2048;
    int offset = 0;

    char rsp[size];
    rsp[offset++] = ESP_RESP_OK;
    rsp[offset++] = type;
    rsp[offset++] = status.waiting_time & 0xFF;
    rsp[offset++] = status.waiting_time >> 8;

    //TODO verify if passing more than 1 byte values like this is correct
    rsp[offset++] = mac_data.successes & 0xFF;
    rsp[offset++] = mac_data.successes >> 8;
    rsp[offset++] = mac_data.failures & 0xFF;
    rsp[offset++] = mac_data.failures >> 8;

    uint32_t saved_retries =  mac_data.retries;
    
    /*
    uint8_t f = (uint8_t) (saved_retries & 0xFF);
    uint8_t s = (uint8_t) ((saved_retries >> 8) & 0xFF ) ;
    uint8_t t = (uint8_t) ((saved_retries >> 16) & 0xFF) ;
    uint8_t fo= (uint8_t) ((saved_retries >> 24) & 0xFF) ;

    Serial.printf("HELLO: %u; %u; %u; %; non cast value: %u\n", f, s, t, fo, saved_retries & 0xFF);
    */

    int retry_offset = offset;
    rsp[offset++] = (uint8_t) (saved_retries & 0xFF);
    rsp[offset++] = (uint8_t) ((saved_retries >> 8) & 0xFF );
    rsp[offset++] = (uint8_t) ((saved_retries >> 16) & 0xFF);
    rsp[offset++] = (uint8_t) ((saved_retries >> 24) & 0xFF);

    //Serial.printf("Retries: %u. Buffer value: %x; %x; %x; %x\n", saved_retries, rsp[retry_offset], rsp[retry_offset + 1], rsp[retry_offset + 2], rsp[retry_offset + 3]);

    uint32_t running_time = (uint32_t) micros() - mac_data.startTime; 
    
    rsp[offset++] = (uint8_t) (running_time & 0xFF);
    rsp[offset++] = (uint8_t) ((running_time >> 8) & 0xFF);
    rsp[offset++] = (uint8_t) ((running_time >> 16) & 0xFF);
    rsp[offset++] = (uint8_t) ((running_time >> 24) & 0xFF);
    

    memcpy(rsp+offset, status.destination_mac_address, MAC_ADDRESS_SIZE);

    int msg_len;

    if(status.message != NULL){
        msg_len = status.content_length;
        memcpy(rsp+offset+MAC_ADDRESS_SIZE, status.message, msg_len);
    } else{
        char without_message[] = "No message given";
        msg_len = strlen(without_message) + 1;
        memcpy(rsp+offset+MAC_ADDRESS_SIZE, without_message, msg_len);
    }
    
    //Status byte + type + time + mac + msg
    client->write( rsp, 1 + sizeof(type) + sizeof(status.waiting_time) + MAC_ADDRESS_SIZE + msg_len );
}

void wifi_handle_message(WiFiClient* client, uint8_t* buffer, uint16_t len){
    char rsp[] = ".Error setting message.";
    if( len < 1 ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }

    PRINT("Len: ");
    PRINTLN_VALUE(len);
    PRINT("New Message: ");
    PRINTLN(buffer);

    macProtocolParameters message_parameter; 
    message_parameter.traf_gen_data.used = true;

    uint16_t message_size = (uint16_t) strlen((char*) buffer) + 1; // plus 1 for '\0' char
    char* message = (char*) malloc(message_size);
    message_parameter.traf_gen_data.message_length = message_size;
    message_parameter.traf_gen_data.message = message;

    memcpy(
        message_parameter.traf_gen_data.message,
        buffer,
        message_size
    );

    //free last
    if(status.message != NULL){
        free(status.message);
    }

    //state change
    status.message = (char*) malloc(message_size);
    status.content_length = message_size;
    memcpy(
        status.message,
        buffer,
        message_size
    );
    
    xQueueSend(protocolParametersQueueHandle, &message_parameter, portMAX_DELAY);

    rsp[0] = ESP_RESP_OK;
    client->write( rsp, 1 );
}

void wifi_handle_time(WiFiClient* client, uint8_t* buffer, uint16_t len){
    char rsp_type[] = ".Invalid interval type.";
    char type = buffer[0];
    if( type != 'c' && type != 'g' ){
        rsp_type[0] = ESP_RESP_ERROR;
        client->write( rsp_type, strlen(rsp_type) );
    }

    char rsp[] = ".Invalid time";
    if( len-1 != 2 ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }

    uint16_t time = ( ((uint16_t)buffer[1]) <<8) + (uint16_t)buffer[2];

    macProtocolParameters time_parameter; 
    time_parameter.traf_gen_time.used = true;

    switch(type){
        case 'c':
            time_parameter.traf_gen_time.time_mode = TRF_GEN_CONST;
            status.time_mode = TRF_GEN_CONST; // status update
            break;
        case 'g':
            time_parameter.traf_gen_time.time_mode = TRF_GEN_GAUSS;
            status.time_mode = TRF_GEN_GAUSS; // status update
    }
    time_parameter.traf_gen_time.waiting_time = time;
    status.waiting_time = time; // status update

    xQueueSend(protocolParametersQueueHandle, &time_parameter, portMAX_DELAY);

    if( time < 50 ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }
    
    PRINT("Len: ");
    PRINTLN_VALUE(len);
    PRINT("New Time: ");
    PRINTLN_VALUE(time);

    rsp[0] = ESP_RESP_OK;
    client->write( rsp, 1 );
}

void wifi_handle_destination(WiFiClient* client, uint8_t* buffer, uint16_t len){
    char rsp[] = ".Wrong length for MAC address.";
    if( len != MAC_ADDRESS_SIZE ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }

    #ifdef MONITOR_DEBUG_MODE
    char dst_mac[MAC_STR_LEN + 1];
    sprintf(dst_mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    PRINT("New Dest: ");
    PRINTLN_VALUE(dst_mac);
    #endif

    macProtocolParameters address_parameter; 
    address_parameter.traf_gen_addr.used = true;

    memcpy(address_parameter.traf_gen_addr.address, buffer, 6);
    memcpy(status.destination_mac_address, buffer, 6);

    xQueueSend(protocolParametersQueueHandle, &address_parameter, portMAX_DELAY);  

    rsp[0] = ESP_RESP_OK;
    client->write( rsp, 1 );
}

void wifi_handle_backoff(WiFiClient* client, uint8_t* buffer, uint16_t len){
    //assuming buffer comes with single char
    char rsp[] = ".Wrong length for backoff protocol";
    if( len != sizeof(char) ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }

    char protocol_char = buffer[0];

    macProtocolParameters backoff_parameter; 
    backoff_parameter.csma_contrl_params.used = true;

    switch (protocol_char){
        case 'm':
            backoff_parameter.csma_contrl_params.backoff_protocol = MILD;
            break;

        case 'n':
            backoff_parameter.csma_contrl_params.backoff_protocol = NON_EXISTANT;
            break;

        case 'l':
            backoff_parameter.csma_contrl_params.backoff_protocol = LINEAR;
            break;
        
        default:
            char protocol_not_recognized_rsp[] = ".Protocol given not recognized";
            rsp[0] = ESP_RESP_ERROR;
            client->write(protocol_not_recognized_rsp, strlen(protocol_not_recognized_rsp));
            return; //not needed from other examples?
    }

    xQueueSend(protocolParametersQueueHandle, &backoff_parameter, portMAX_DELAY);

    rsp[0] = ESP_RESP_OK;
    client->write( rsp, 1 );

}

void wifi_choose_handler(WiFiClient* client, uint8_t* buffer, uint16_t len){
    for (int i = 0; i < NUM_OPTIONS; ++i) {

        if(options[i].opt_code != buffer[0]){
            continue;
        }
        
        // Opt Code becomes irrelevant from here on out
        options[i].handler(client, buffer+1, len-1);
        break;
    }
}

void wifi_com_handle_con(WiFiServer* server){

    while(true){

        WiFiClient client = server->accept();

        if (client) {
            Serial.println("Client connected.");

            // Wait until the client sends data
            while (client.connected()) {
                if (client.available()) {
                    // Read data from client
                    uint16_t len = 0;
                    client.readBytes((char*)&len, sizeof(len)); // Read length

                    len -= 2; //Already read the first 2 bytes;
                    PRINT("Length = ");
                    PRINTLN_VALUE(len);

                    uint8_t buffer[len];
                    client.readBytes((char*)buffer, len);

                    wifi_choose_handler(&client, buffer, len);

                }
            }

            // Client disconnected
            client.stop();
            Serial.println("Client disconnected.");
        }

        delay(10);
    }

}