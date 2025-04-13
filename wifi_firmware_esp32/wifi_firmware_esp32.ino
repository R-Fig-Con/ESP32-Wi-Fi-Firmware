#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"

CC1101 radio;

byte syncWord[2] = {199, 10};
volatile bool packetWaiting;

uint8_t myMacAddress[6];

// Packet and frame used by the sender.
TRAFFIC_GEN * trf_gen;

CSMA_CONTROL * csma_control;


TaskHandle_t receiveHandle = NULL;

void messageReceived() {
    packetWaiting = true;
    /*
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(receiveHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    */

    //vTaskResume(receiveHandle);
}

void receiveTask(void* unused_param){

  while(true){

    vTaskSuspend(receiveHandle);

    receiver();

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Pause before next check

  }
      
}

bool checkChannel(){
  radio.setRxState();//maybe not necessary
  
  return radio.cca();
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

    csma_control = new CSMA_CONTROL(&checkChannel);


    attachInterrupt(CC1101_GDO0, messageReceived, RISING);
    
    uint8_t dstMacAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    trf_gen = new TRAFFIC_GEN(&sender, myMacAddress, dstMacAddress);
    trf_gen->setTime(TRF_GEN_GAUSS, 6000);
    
    /*
    //creating when there is nothing to receive may cause troubles
    xTaskCreatePinnedToCore(
      &receiveTask,
      "receive and send acknowledge",
      10000, //no thought used to decide size
      NULL,
      10, // believe it should be bigger than traffic generation
      &receiveHandle,
      1 //putting related to cc1101 on same core
    );
    */

    // Make sure the radio is on RX.
    radio.setRxState();
}

TaskHandle_t generatorHandle = NULL;

void generatorTask(void* unusedParam){
  trf_gen->init();//is already a loop. Maybe change to add init restart on isRunning == false?
}

void loop(){
    if(!trf_gen->isRunning()){
        Serial.println(F("Initiating traffic...")); 
        xTaskCreatePinnedToCore(
          &generatorTask,     // Function to execute
          "traffic generator",   // Name of the task
          10000,      // Stack size
          NULL,      // Task parameters
          1,         // Priority
          &generatorHandle,      // Task handle
          1          // Core 1
        );
    }
}

CCPACKET packet_to_receive;
ieeeFrame * frame = (ieeeFrame *) packet_to_receive.data;

/**
 * requires radio initialization, csma control instance, receive task creation
 * Contains retry logic for now at least
 */
void sender(CCPACKET packet_to_send) { 

  uint8_t retryCount = 0;

  send:

  detachInterrupt(CC1101_GDO0);

  if(retryCount == 10){
    Serial.println(F("GIVING UP after retry limit reached"));
    return;
  }

  csma_control->waitForTurn();

  bool b = radio.sendData(packet_to_send);
  if(!b){//in protocol should not expect failure here
    Serial.println(F("Send failed."));
    retryCount += 1;
    goto send;
  }

  unsigned long start_time = micros();
  Serial.println(F("Packet sent"));

  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  while(!packetWaiting){
    //sifs wait
    if(micros() - start_time >= 3500){
      retryCount += 1;
      goto send;
    }

  }


  receiver();

  //checks if is ack
  if(!PACKET_IS_ACK(frame)){
    retryCount += 1;
    goto send;
  }

  //to eventually add sucess to register, so it can be accessed by the esp wifi

  Serial.println(F("Complete success"));
}

/**
 * Directly affects 'packet_to_receive' global var
 */
void receiver() {

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  radio.receiveData(&packet_to_receive);

  Serial.println(F("Received packet..."));

  // We received something, but is the packet correct?
  if (!packet_to_receive.crc_ok) {
      Serial.println(F("crc not ok"));
  }

  /*
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
  */
  

  // Mark the packet as processed and re-enable the interruption.
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

}