#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"


#include <WiFi.h>

/**
 * Communication with computer groundwork.
 * Contents of main file can be substitued by this content
*/

const char *ssid = "..."; // internet name
const char *password = "..."; //internet password. Believe it is null if none is used

NetworkServer server = NetworkServer(1234);

IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("BEGUN"));
  //WiFi.softAP(ssid, password);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(F("ESP32 connected, to start server"));
  server.begin();
}

void loop() {
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
    client.stop();
    Serial.println("Client disconnected");
  }
}
