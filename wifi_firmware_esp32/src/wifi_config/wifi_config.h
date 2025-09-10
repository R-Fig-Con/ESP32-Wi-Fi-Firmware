#include <WiFi.h>
#include <esp_wifi.h>
#include "wifi_config_app_com.h"

#include "internet_info.h" //Hidden by gitignore, recreate it 

#define AP_PORT 5000

#define MAC_STR_LEN (int)(MAC_ADDRESS_SIZE*2 + (MAC_ADDRESS_SIZE-1))

void wifi_com_task(void* parameter);

bool wifi_com_start(WiFiServer* server, uint8_t my_mac[MAC_ADDRESS_SIZE]);

void wifi_com_handle_con(NetworkServer server);