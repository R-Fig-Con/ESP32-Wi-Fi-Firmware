#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"

CC1101 radio;

byte syncWord[2] = {199, 10};

uint8_t myMacAddress[6];

//test file forcca through gdo2 pin
//use gdo2 read on cca and uncomment all setMonitorCCA, in send, receive and reset
//From experiment, seems to get stuck when it goes from free to busy
//change id var to change mode

void setup() {

    // Serial communication for debug
    Serial.begin(115200);

    // Wifi, for getting the MAC address.
    WiFi.mode(WIFI_STA);
    WiFi.STA.begin();
    esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);

    // Initialize the CC1101 radio
    radio.init();
    radio.setSyncWord(syncWord);
    radio.setCarrierFreq(CFREQ_433);
    radio.disableAddressCheck();
    radio.setTxPowerAmp(PA_LowPower);

    delay(1000);

    // Print some debug info
    Serial.print(F("CC1101_PARTNUM "));
    Serial.println(radio.readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER));
    Serial.print(F("CC1101_VERSION "));
    Serial.println(radio.readReg(CC1101_VERSION, CC1101_STATUS_REGISTER));
    Serial.print(F("CC1101_MARCSTATE "));
    Serial.println(radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);
    
}

//int id = 1;
int id = 0;

void loop(){
  if(id){
    delay(20000);
    Serial.println(F("JAMMING"));
    radio.jamming();
  }
  else{
    bool free = radio.cca();
    Serial.print(F("Read cca; value:"));
    Serial.println(free);
    delay(200);
  }
  
}