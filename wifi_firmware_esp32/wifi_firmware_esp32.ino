#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"

enum nodeOpMode {

  NODE_OP_MODE_RECEIVER,
  NODE_OP_MODE_TRANSMITTER
  
};

CC1101 radio;

byte syncWord[2] = {199, 10};
volatile bool packetWaiting;

nodeOpMode opMode;
uint8_t myMacAddress[6];
CCPACKET packet_to_send;


unsigned long time_marker = 0;

void messageReceived() {
    time_marker = micros();
    //Serial.printf("ON INTERRUPT marker = %lu\n", time_marker);//CAUSES FAILURE???
    packetWaiting = true;
}


void setup() {
    // Serial communication for debug
    Serial.begin(115200);

    delay(1000);

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

    if (opMode == NODE_OP_MODE_RECEIVER) Serial.printf("Works as receiver, marker: %lu\n", time_marker);
    else {sender_create_data_packet(&packet_to_send); Serial.printf("Works as receiver\n");}

    // Make sure the radio is on RX.
    radio.setRxState();

    #ifdef ESP32
    Serial.println("ESP32 is defined");

    #else
    Serial.println(F("Not defined"));
    #endif 
}

CCPACKET packet_to_receive;//also to send
void loop(){
  
    if(opMode == NODE_OP_MODE_TRANSMITTER){
        sendAndReceive();
        delay(40000);
    }else if(opMode == NODE_OP_MODE_RECEIVER){
        receiveAndAcknowledge();
    }
    
}


ieeeFrame * frame = (ieeeFrame *) packet_to_receive.data;

void receiveAndAcknowledge(){
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);
  Serial.println(F("TO WAIT FOR PACKET"));
  while(!packetWaiting);
  receiver();

  packet_to_receive.length = 0;
  PACKET_TO_ACK(frame);


  radio.sendData(packet_to_receive);
  //radio.sendWithWaitPrint(packet_to_receive);

  Serial.printf("PACKET SENT, state: %d in core %d\n", (int) radio.readStatusReg(CC1101_MARCSTATE), xPortGetCoreID());

}


void sender_create_data_packet(CCPACKET * packet) {
  unsigned int seqNum = 0;
  ieeeFrame * f = (ieeeFrame *) packet->data;

  // Fill the MAC address.
  memcpy(f->addr_src, myMacAddress, 6);

  // Fill the payload. Start by adding a sequence number.
  // TODO: We will certainly want this sequence number to be 
  // binary in the final version. We are using strings
  // here just ti simplify debug.
  sprintf((char *) f->payload, "%08X ", seqNum++);

  // Now, fill the remainder of the payload with some 
  // data (just to have something that is verificable on
  // the receiver end).
  // TODO: this can be safely removed on the final version,
  // as the payload itself is not going to be important.
  int l = strlen((char *) f->payload);
  int i;
  for (i = 0; i < 1000; i++) {

    f->payload[l++] = (char) ((i % 42) + 48);
  }
  f->payload[l] = (char) 0;
  
  // Set the packet length. It is the length of the payload
  // because it is a string, remember to count the \0 at the end,
  // plus the 6 bytes of the MAC address.
  packet->length = strlen((char *) f->payload)  + 1 + 6;
}
 
void sendAndReceive(){
  Serial.printf("Send and receive, marker: %lu\n", time_marker);
  radio.sendData(packet_to_send);
  unsigned long wait_start = micros();
  //Serial.printf("2nd Send and receive, marker: %lu\n", time_marker);
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  unsigned long c = 0;
  while(!packetWaiting){
    Serial.println(F("LOOP\n"));//Necesary to detect? TODO CHECK
    c += 1;
  }

  Serial.printf("Initial sender; Took %lu microsseconds from packet send to answer, loop count %lu;\n", time_marker - wait_start, c);

  time_marker = 0;
  detachInterrupt(CC1101_GDO0);

}

void receiver() {

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  if (radio.receiveData(&packet_to_receive) > 0) {

    //Serial.println(F("Received packet..."));

    // We received something, but is the packet correct?
    if (!packet_to_receive.crc_ok) {
        Serial.println(F("crc not ok"));
    }

    /*
    Serial.printf("STATE ON MAIN BEFORE OTHER PRINTS: %d\n", (int) radio.readStatusReg(CC1101_MARCSTATE));

    // Print some physical layer statistics
    Serial.print(F("lqi: "));
    Serial.println(radio.raw2lqi(packet_to_receive.lqi));
    Serial.print(F("rssi: "));
    Serial.print(radio.raw2rssi(packet_to_receive.rssi));
    Serial.println(F("dBm"));

    // If the packet seems right, let's process it.
    if (packet_to_receive.length > 0) {

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
    

    Serial.printf("STATE ON MAIN AFTER OTHER PRINTS: %d\n", (int) radio.readStatusReg(CC1101_MARCSTATE));

    */

  }

  // Mark the packet as processed and re-enable the interruption.
  packetWaiting = false;

}