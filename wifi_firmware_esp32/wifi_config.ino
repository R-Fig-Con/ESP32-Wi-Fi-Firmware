//#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager


//QueueHandle_t protocolParametersQueueHandle = xQueueCreate( 1, sizeof(macProtocolParameters) );
WiFiServer server(PORT);
//WiFiManager wm;

void wifi_com_task(void* parameter) {
    uint8_t* mac_addr = (uint8_t*) parameter;

    if (wifi_com_start(mac_addr)) {
        Serial.println("WiFi COM started successfully.");
    } else {
        Serial.println("WiFi COM failed to start.");
    }

    macProtocolParameters params;

    while (true){
        NetworkClient client = server.available();
        if (client) {
            Serial.println("Client connected");
            while (client.connected()) {
                if (client.available()) {
                String data = client.readStringUntil('\n');
                Serial.print("Received: ");
                Serial.println(data);
                client.print("Hello from ESP32!\n");
                }
            }
        //no data collection
        params.csma_contrl_params.used = true;
        params.csma_contrl_params.backoff_protocol = MILD;
        xQueueSend(protocolParametersQueueHandle, (const void*) &params, portMAX_DELAY);
        client.stop();
        Serial.println("Client disconnected");
        }
        
    }
    
}

bool wifi_com_start(uint8_t my_mac[MAC_ADDRESS_SIZE]){
    // Define static IP and network range
    IPAddress local_IP(10, 0, 0, 1);      // ESP32 AP IP
    IPAddress gateway(10, 0, 0, 1);       // Same as local IP
    IPAddress subnet(255, 255, 255, 0);   // Subnet mask

    char ssid[6 + MAC_STR_LEN + 1] = "ESP32-"; //Base(6) + MAC(6) + null terminator
    sprintf(ssid + 6, "%02X%02X%02X%02X%02X%02X", 
        my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

    const char* password = WIFI_PASSWORD;

    Serial.printf("SSID: %s\n", ssid);

    WiFi.mode(WIFI_MODE_AP);

    // Configure static IP for the SoftAP interface
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("WiFi Config Failed!");
        return false;
    }

    // Start the SoftAP with SSID and password
    if (!WiFi.softAP(ssid, password)) {
        Serial.println("WiFi Start Failed!");
        return false;
    }

    //wm.setConfigPortalBlocking(false);
    //wm.setConfigPortalTimeout(60);

    server.begin();

    Serial.printf("Listening on %s:%d\n", WiFi.softAPIP().toString(), PORT);

    return true;
}