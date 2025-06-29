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
    //TODO
}

void wifi_handle_message(WiFiClient* client, uint8_t* buffer, uint16_t len){
    //TODO
}

void wifi_handle_time(WiFiClient* client, uint8_t* buffer, uint16_t len){
    char rsp_type[] = ".Invalid interval type.";
    char type = buffer[0];
    if( type != 'c' && type != 'g' ){
        rsp_type[0] = ESP_RESP_ERROR;
        client->write( rsp_type, strlen(rsp_type) );
    }

    char rsp[] = ".Invalid time 1.";
    if( len-1 != 2 ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }

    uint16_t time = ( ((uint16_t)buffer[1]) <<8) + (uint16_t)buffer[2];

    char error2[] = ".Invalid time 2.";
    if( time < 50 ){
        error2[0] = ESP_RESP_ERROR;
        client->write( error2, strlen(error2) );
    }
    //For DEBUG
    Serial.printf("Len: %d\nTime 1 = %u || Time 2 = %u\nNew Time: %u\n", len, buffer[1], buffer[2], time);
    //---------

    //TODO(Set Time in traffic generator)

    rsp[0] = ESP_RESP_OK;
    client->write( rsp, 1 );
}

void wifi_handle_destination(WiFiClient* client, uint8_t* buffer, uint16_t len){
    char rsp[] = ".Wrong length for MAC address.";
    if( len != MAC_ADDRESS_SIZE ){
        rsp[0] = ESP_RESP_ERROR;
        client->write( rsp, strlen(rsp) );
    }

    //For DEBUG
    char dst_mac[MAC_STR_LEN + 1];
    sprintf(dst_mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

    Serial.printf("New Dest: %s\n", dst_mac);
    //---------

    //TODO(Set Dest in traffic generator)

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
                    Serial.printf("Length = %d\n", len); //Debug

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