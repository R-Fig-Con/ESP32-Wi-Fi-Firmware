#include <ESPmDNS.h>

void wifi_com_task(void* parameter) {
    uint8_t* mac_addr = (uint8_t*) parameter;

    WiFiServer server(AP_PORT);
    bool status = wifi_com_start(&server, mac_addr);

    if (status) {
        Serial.println("WiFi COM started successfully.");
        wifi_com_handle_con(&server);
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
    BACKOFF_PROTOCOLS protocol = DEFAULT_BACKOFF_ALGORITHM;

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

bool wifi_com_start(WiFiServer* server, uint8_t my_mac[MAC_ADDRESS_SIZE]){

    const char* ssid = WIFI_NAME;
    const char* password = WIFI_PASSWORD;

    WiFi.mode(WIFI_MODE_STA);

    // Start the SoftAP with SSID and password
    if (!WiFi.begin(ssid, password)) {
        Serial.println("WiFi Start Failed!");
        return false;
    }

    //From mdns example
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500); Serial.print(".");
    }

    // On begin name collision chooses esp32-2 (and probably so on)
    if (!MDNS.begin(WiFi.localIP().toString().c_str())) {
      Serial.println("Error setting up MDNS responder!");
      return false; // TODO maybe add different returns for each error
    }
    Serial.println("mDNS responder started");

    // Start TCP (HTTP) server
    server->begin();
    Serial.println("TCP server started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", AP_PORT); //TODO maybe not call it http?

    Serial.printf("Listening on %s; ruuning on port %d but addService on port 80\n", WiFi.localIP().toString().c_str(), AP_PORT);

    return true;
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

    //int failure_offset = offset; - not being used
    uint16_t saved_failures = mac_data.failures;

    rsp[offset++] = saved_failures & 0xFF;
    rsp[offset++] = saved_failures >> 8;

    uint32_t saved_retries =  mac_data.retries;

    //int retry_offset = offset; - not being used
    rsp[offset++] = (uint8_t) (saved_retries & 0xFF);
    rsp[offset++] = (uint8_t) ((saved_retries >> 8) & 0xFF );
    rsp[offset++] = (uint8_t) ((saved_retries >> 16) & 0xFF);
    rsp[offset++] = (uint8_t) ((saved_retries >> 24) & 0xFF);

    //Serial.printf("Retries: %u. Buffer value: %x; %x; %x; %x\n", saved_retries, rsp[retry_offset], rsp[retry_offset + 1], rsp[retry_offset + 2], rsp[retry_offset + 3]);

    uint32_t running_time = (uint32_t) millis() - mac_data.startTime; 
    
    rsp[offset++] = (uint8_t) (running_time & 0xFF);
    rsp[offset++] = (uint8_t) ((running_time >> 8) & 0xFF);
    rsp[offset++] = (uint8_t) ((running_time >> 16) & 0xFF);
    rsp[offset++] = (uint8_t) ((running_time >> 24) & 0xFF);

    memcpy(rsp+offset, status.destination_mac_address, MAC_ADDRESS_SIZE);

    offset += MAC_ADDRESS_SIZE;

    int msg_len;

    if(status.message != NULL){
        msg_len = status.content_length;
        memcpy(rsp+offset, status.message, msg_len);
    } else{
        char without_message[] = "No message given";
        msg_len = strlen(without_message) + 1;
        memcpy(rsp+offset, without_message, msg_len);
    }

    offset += msg_len;
    
    client->write( rsp, 1 + offset );
}

void wifi_handle_message(WiFiClient* client, uint8_t* buffer, uint16_t len){
    char rsp[] = ".Error setting message.";
    if( len < 1 ){
        Serial.printf("Error setting message, length too small (length of %u)\n", len);
        Serial.printf("Message received: %s\n", buffer);
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
        return;
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
        Serial.println("Error time set; Invalid type");
        rsp_type[0] = ESP_RESP_ERROR;
        client->write( rsp_type, strlen(rsp_type) );
        return;
    }

    char rsp[] = ".Invalid time";
    if( len-1 != 2 ){
        Serial.println("Error time set; Invalid length?");
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
        return;
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

    if( time < 50 ){
        Serial.println("Error time set; too small");
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
        return;
    }

    time_parameter.traf_gen_time.waiting_time = time;
    status.waiting_time = time; // status update

    xQueueSend(protocolParametersQueueHandle, &time_parameter, portMAX_DELAY);
    
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
        Serial.println("Address error; wrong size");
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
        return;
    }

    PRINT("New Dest: ");
    PRINTLN_MAC(buffer);

    macProtocolParameters address_parameter; 
    address_parameter.traf_gen_addr.used = true;

    memcpy(address_parameter.traf_gen_addr.address, buffer, MAC_ADDRESS_SIZE);
    memcpy(status.destination_mac_address, buffer, MAC_ADDRESS_SIZE);

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
        Serial.printf("Backoff error; received message with length %u, expected one with size 1\n", len);
        return;
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
            Serial.printf("Backoff error; received char %c, not recognized\n", protocol_char);
            char protocol_not_recognized_rsp[] = ".Protocol given not recognized";
            protocol_not_recognized_rsp[0] = ESP_RESP_ERROR;
            client->write(protocol_not_recognized_rsp, strlen(protocol_not_recognized_rsp));
            return;
    }

    xQueueSend(protocolParametersQueueHandle, &backoff_parameter, portMAX_DELAY);
    Serial.println("Sent backoff parameters");

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

void wifi_com_handle_con(NetworkServer* server){

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