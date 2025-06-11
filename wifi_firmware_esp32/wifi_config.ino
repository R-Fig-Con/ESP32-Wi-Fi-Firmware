void wifi_com_task(void* parameter) {
    uint8_t* mac_addr = (uint8_t*)parameter;

    bool success = wifi_com_start(mac_addr);

    if (success) {
        Serial.println("WiFi COM started successfully.");
    } else {
        Serial.println("WiFi COM failed to start.");
    }

    // Optional: delete the task if it's done
    vTaskDelete(NULL);
}

bool wifi_com_start(uint8_t my_mac[MAC_ADDRESS_SIZE]){
    // Define static IP and network range
    IPAddress local_IP(10, 0, 0, 1);      // ESP32 AP IP
    IPAddress gateway(10, 0, 0, 1);       // Same as local IP
    IPAddress subnet(255, 255, 255, 0);   // Subnet mask

    WiFiServer server(PORT);

    char ssid[6 + MAC_STR_LEN + 1] = "ESP32-"; //Base(6) + MAC(6) + null terminator
    sprintf(ssid + 6, "%02X%02X%02X%02X%02X%02X", 
        my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

    const char* password = "YourPassword";

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

    server.begin();

    Serial.println("WiFi AP is now enabled.");
    Serial.printf("Listening on IP: %s Port: %d\n", WiFi.softAPIP().toString(), PORT);

    return true;
}