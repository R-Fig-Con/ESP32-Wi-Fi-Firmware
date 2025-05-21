#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"

CC1101 radio;

uint8_t myMacAddress[6];

// Packet and frame used by the sender.
CCPACKET packet;
ieeeFrame * frame = (ieeeFrame *) packet.data;

bool packetWaiting;

void messageReceived() {
    packetWaiting = true;
}

//int id = 1;
int id = 0;

void setup() {

    // Serial communication for debug
    Serial.begin(115200);

    // Wifi, for getting the MAC address.
    WiFi.mode(WIFI_STA);
    WiFi.STA.begin();
    esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);

    // Initialize the CC1101 radio
    radio.init();
    radio.setCarrierFreq(CFREQ_433);
    radio.disableAddressCheck();
    radio.setTxPowerAmp(PA_LowPower);

    delay(1000);

    Serial.print(F("CC1101_MARCSTATE "));
    Serial.println(radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);

    if (id == 0){
      attachInterrupt(CC1101_GDO0, messageReceived, RISING);
    }
    
    
}

int counter = 0;
void loop(){
  
  if (counter == 50){
    Serial.println(F("\n\nReebotting Reebotting Reebotting Reebotting Reebotting Reebotting Reebotting Reebotting"));
    counter = 0;
    /*
    radio.setIdleState();
    while(radio.readStatusReg(CC1101_MARCSTATE) != RFSTATE_IDLE);
    Serial.println(F("SET ON IDLE"));
    radio.flushRxFifo();
    radio.setRxState();
    */
    
    /*
    //detachInterrupt(CC1101_GDO0);
    radio.reset();
    //attachInterrupt(CC1101_GDO0, messageReceived, RISING);
    */

    Serial.println(F("Reeboted Reeboted Reeboted Reeboted Reeboted Reeboted Reeboted Reeboted\n\n"));
  }
  if(id){
    delay(20000);
    Serial.println(F("JAMMING"));
    radio.jamming();
  }
  else{
    counter += 1;
    bool free = radio.cca();
    int8_t rssi = (int8_t) radio.readStatusReg(CC1101_RSSI);
    
    if(free){
      Serial.println(F("\n\nRead cca; is free"));
    }
    else{
      Serial.println(F("\n\nRead cca; is NOT FREE"));
    }

    Serial.print(F("Read rssi; value:"));
    Serial.println(rssi);

    Serial.print(F("Read state; value:"));
    Serial.println(radio.readStatusReg(CC1101_MARCSTATE));

    delay(200);
    receiver();
  }
  
  Serial.println("Loop end");
}

void receiver() {

  // Is there a packet to be received?
  if(!packetWaiting) return ;

  //Serial.println("RECEIVEE");

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  if (radio.receiveData(&packet) > 0)
    Serial.println(F("Received packet..."));

  // We received something, but is the packet correct?
  if (!packet.crc_ok) {
      Serial.println(F("crc not ok"));
  }

  // Print some physical layer statistics
  Serial.print(F("lqi: "));
  Serial.println(radio.raw2lqi(packet.lqi));
  Serial.print(F("rssi: "));
  Serial.print(radio.raw2rssi(packet.rssi));
  Serial.println(F("dBm"));

  // Mark the packet as processed and re-enable the interruption.
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

}