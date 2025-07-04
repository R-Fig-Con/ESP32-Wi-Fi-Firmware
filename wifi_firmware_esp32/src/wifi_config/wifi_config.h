#include <WiFi.h>
#include <esp_wifi.h>
#include "wifi_config_app_com.h"

#define AP_PORT 5000
const uint8_t AP_IP_ARR[4] = {10, 0, 0, 1};

#define MAC_STR_LEN (int)(MAC_ADDRESS_SIZE*2 + (MAC_ADDRESS_SIZE-1))

#define WIFI_PASSWORD "ESP32-firmware"

struct WIFI_CONFIG_RET{
    WiFiServer* server;
    bool success;
};

void wifi_com_task(void* parameter);

WIFI_CONFIG_RET wifi_com_start(uint8_t my_mac[MAC_ADDRESS_SIZE]);

void wifi_com_handle_con(WiFiServer server);