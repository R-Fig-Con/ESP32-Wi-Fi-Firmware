
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>

const char *ssid = "IBG";
const char *password = "b5bb300252";

void setup() {
  Serial.begin(57600);
  delay(2000);
  WiFi.begin(ssid, password);

  //WiFi.mode(WIFI_STA); not to use

  while (WiFi.status() != WL_CONNECTED){  delay(1000);  }

  Serial.println(F("AFTER WHILE LOOP"));
  if (MDNS.begin("esp32")) { //maybe as loop until it finds one not occupied
    Serial.println("mDNS responder started");
  }

  if(MDNS.addService("esp", "tcp", 80)){
    Serial.println(F("SERVICE ADDED"));
  }
  
  String name = MDNS.hostname(0);
  Serial.println(name);
  Serial.println("InstanceName printed");
}

void loop(){
  delay(2000);
  Serial.println(F("LOOP"));  
}