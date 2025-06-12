#ifndef _WIFI_CONF_H
#define _WIFI_CONF_H

#include <WiFi.h>
#include <esp_wifi.h>

#define PORT 5000

#define MAC_STR_LEN (int)(MAC_ADDRESS_SIZE*2)

#define WIFI_PASSWORD "ESP32-firmware"

#endif

void wifi_com_task(void* parameter);

bool wifi_com_start(uint8_t my_mac[MAC_ADDRESS_SIZE]);