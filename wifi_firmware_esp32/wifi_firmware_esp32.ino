#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"

/**
 * Useful constants.
 */

// Node operation mode: transmitter or receiver.
enum nodeOpMode {

  NODE_OP_MODE_RECEIVER,
  NODE_OP_MODE_TRANSMITTER
};

/**
 * Frame format.
 */
struct macFrame {

  // Destination/source address
  uint8_t   addr[6];

  // Payload
  uint8_t   payload[0];
};


/**
 * Global variables
 */
CC1101 radio;

byte syncWord[2] = {199, 10};
bool packetWaiting;

unsigned long lastSend = 0;
unsigned int sendDelay = 1000;
unsigned int seqNum = 0;

nodeOpMode opMode;
uint8_t myMacAddress[6];

// Packet and frame used by the sender.
CCPACKET packet;
macFrame * frame = (macFrame *) packet.data;

void messageReceived() {
    packetWaiting = true;
}

void setup() {

  ////
  // Initialize helper modules

  // Serial communication for debug
  Serial.begin(500000);

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
  opMode = NODE_OP_MODE_TRANSMITTER;

  // If we are operating as a receiver, attach the interrupt
  // for detecting receptions. For the sender, create the packet
  // to be sent.
  if (opMode == NODE_OP_MODE_RECEIVER) attachInterrupt(CC1101_GDO0, messageReceived, RISING);
  else sender_create_data_packet(& packet);

  // Make sure the radio is on RX.
  radio.setRxState();
}

void loop() {

    if (opMode == NODE_OP_MODE_RECEIVER) receiver();
    else sender();
}

/**
 * Sender related functions.
 */

void sender_create_data_packet(CCPACKET * packet) {

  macFrame * frame = (macFrame *) packet->data;

  // Fill the MAC address.
  memcpy(frame->addr, myMacAddress, 6);

  // Fill the payload. Start by adding a sequence number.
  // TODO: We will certainly want this sequence number to be 
  // binary in the final version. We are using strings
  // here just ti simplify debug.
  sprintf((char *) frame->payload, "%08X ", seqNum++);

  // Now, fill the remainder of the payload with some 
  // data (just to have something that is verificable on
  // the receiver end).
  // TODO: this can be safely removed on the final version,
  // as the payload itself is not going to be important.
  int l = strlen((char *) frame->payload);
  int i;
  for (i = 0; i < 1000; i++) {

    frame->payload[l++] = (char) ((i % 42) + 48);
  }
  frame->payload[l] = (char) 0;
  
  // Set the packet length. It is the length of the payload
  // because it is a string, remember to count the \0 at the end),
  // plus the 6 bytes of the MAC address.
  packet->length = strlen((char *) frame->payload)  + 1 + 6;
}

void sender() {

  unsigned long now = millis();
      
  // Is it time to generate the next packet?
  if (now < lastSend + sendDelay) return;

  // Yes. Store the current time to use as a reference for the next packet.
  lastSend = now;

  bool b = radio.sendData(packet);
  if(b)
    Serial.println(F("Packet sent."));
  else
    Serial.println(F("Sent failed."));
}

void receiver() {

  // Is there a packet to be received?
  if(!packetWaiting) return ;

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  if (radio.receiveData(&packet) > 0) {

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

    // If the packet seems right, let's process it.
    if (packet.crc_ok && packet.length > 0) {

      // Just print some debug info.
      Serial.print(F("packet: len "));
      Serial.println(packet.length);
      Serial.println(F("data: "));
      Serial.printf("src: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            frame->addr[0], 
            frame->addr[1], 
            frame->addr[2], 
            frame->addr[3], 
            frame->addr[4], 
            frame->addr[5]);
      Serial.println((const char *) frame->payload);
    }
  }

  // Mark the packet as processed and re-enable the interruption.
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

}

