#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"

enum nodeOpMode {

  NODE_OP_MODE_RECEIVER,
  NODE_OP_MODE_TRANSMITTER
  
};

CC1101 radio;

byte syncWord[2] = {199, 10};
bool packetWaiting;

nodeOpMode opMode;
uint8_t myMacAddress[6];

// Packet and frame used by the sender.
TRAFFIC_GEN * trf_gen;

void messageReceived() {
    //Serial.println(F("ON INTERRUPT")); //Penso que este tipo de operação nã deve ser feito num interrupt, o interrupt deve ser o mais rápido possível
    packetWaiting = true;
}

//Task for other core
void generatorLoop( void * parameter) {
  for(;;) {
    Serial.println(F("ON TASK"));
    trf_gen->init();
  }
}

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

    Serial.println(F("CC1101 radio initialized."));

    // Select this node's operation mode
    opMode = NODE_OP_MODE_RECEIVER;
    //opMode = NODE_OP_MODE_TRANSMITTER;

    if (opMode == NODE_OP_MODE_RECEIVER) Serial.println(F("Works as receiver"));
    else Serial.println(F("Works as transmitter"));

    // If we are operating as a receiver, attach the interrupt
    // for detecting receptions. For the sender, create the packet
    // to be sent.
    if (opMode == NODE_OP_MODE_RECEIVER) attachInterrupt(CC1101_GDO0, messageReceived, RISING);
    else {
        uint8_t dstMacAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        trf_gen = new TRAFFIC_GEN(&sender, myMacAddress, dstMacAddress);
        //trf_gen->setTime(TRF_GEN_GAUSS, 3000);
    }

    // Make sure the radio is on RX.
    radio.setRxState();

/*
  //https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/ 
  xTaskCreatePinnedToCore(
    generatorLoop, // Function to implement the task
    "Regular transmitter", // Name of  task
    10000,  // Stack size in words
    NULL,  // Task input parameter, should be none for module
    0,  // Priority of the task, if is only on core i imagine it does not matter, but should be checked
    NULL,  // Task handle. Non null structs could be used to later delete the task
    0   // Core where the task should run. Used 0 since 1 is already occupied by loop on default
  ); 
  */
}

void loop(){
  /*
    if(opMode == NODE_OP_MODE_TRANSMITTER && !trf_gen->isRunning()){
        Serial.println(F("Initiating traffic...")); 
        trf_gen->init(); //TODO: Launch in different core
    }else if(opMode == NODE_OP_MODE_RECEIVER){
        receiver();
    }
    */
    unsigned long a = micros();
    bool res = radio.cca();
    unsigned long end = micros();
    Serial.printf("Channel free: %d; Took %ul microsseconds to check\n", res, end - a);

    delay(200);
}

void sender(CCPACKET packet_to_send) { 

  bool b = radio.sendData(packet_to_send);
  if(b){
    Serial.print(F("Packet sent; payload: ")); 
    Serial.println((char *) ( ((ieeeFrame *)(packet_to_send.data))->payload));
  }else
    Serial.println(F("Send failed."));
}

CCPACKET packet_to_receive;
ieeeFrame * frame = (ieeeFrame *) packet_to_receive.data;

void receiver() {

  // Is there a packet to be received?
  if(!packetWaiting) return ;

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  if (radio.receiveData(&packet_to_receive) > 0) {

    Serial.println(F("Received packet..."));

    // We received something, but is the packet correct?
    if (!packet_to_receive.crc_ok) {
        Serial.println(F("crc not ok"));
    }

    // Print some physical layer statistics
    Serial.print(F("lqi: "));
    Serial.println(radio.raw2lqi(packet_to_receive.lqi));
    Serial.print(F("rssi: "));
    Serial.print(radio.raw2rssi(packet_to_receive.rssi));
    Serial.println(F("dBm"));

    // If the packet seems right, let's process it.
    if (/*packet_to_receive.crc_ok && */packet_to_receive.length > 0) {

      // Just print some debug info.
      Serial.print(F("packet: len "));
      Serial.println(packet_to_receive.length);
      Serial.printf("src: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            frame->addr_src[0],
            frame->addr_src[1],
            frame->addr_src[2],
            frame->addr_src[3],
            frame->addr_src[4],
            frame->addr_src[5]);
      Serial.println(F("data: "));
      Serial.println((const char *) frame->payload);
    }
  }

  // Mark the packet as processed and re-enable the interruption.
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

}