void wifi_com_task(void* parameter) {
    uint8_t* mac_addr = (uint8_t*)parameter;

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

    char ssid[6 + MAC_STR_LEN + 1] = "ESP32-"; // Base(6) + MAC(6) + null terminator
    sprintf(ssid + 6, "%02X%02X%02X%02X%02X%02X", 
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

    Serial.printf("Listening on %s:%d\n", WiFi.softAPIP().toString(), AP_PORT);

    ret.success = true;
    return ret;
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
                    String message = client.readStringUntil('\n'); // Read until newline
                    Serial.print("Received: ");
                    Serial.println(message);

                    // Process message or respond
                    client.print("Message received on the ESP!");
                }
            }

            // Client disconnected
            client.stop();
            Serial.println("Client disconnected.");
        }

        delay(10);
    }

}