#include <Arduino.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"
#include "src/wifi_config/wifi_config.h"

/**
 * to uncomment if strategy of changing cpu cores works and is to be used
*/
//#define CACHE_CHANGE_IN_USE  "Irrelevant value"

/**
 * to uncomment if answer task logic changes with mac protocol parameter change
*/
//#define ANSWER_TASK_CHANGES_WITH_PARAMETERS "Irrelevant value"

//cache change currently not working

// From https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf#sysmem
// Allows to switch cache of each processor, used to ensure radio mac protocol contains new and updated cache values
//check section 3.3.4 for mutex explanation and 12.4 for register address
#define DPORT_CACHE_MUX_MODE_REG_ADDR 0x3FF0007C
#define DPORT_CACHE_MUX_MODE_REG_DEF_VAL 0
#define DPORT_CACHE_MUX_MODE_REG_SWITCH_VAL 3

/**
 * dont know if it needs to be volatile, is just in case
*/
volatile uint32_t* cacheRegisterMutex = (volatile uint32_t*) DPORT_CACHE_MUX_MODE_REG_ADDR;


/**
 * multiple increase, linear decrease
*/
class MILD_BACKOFF: public CONTENTION_BACKOFF{

  void reduceContentionWindow(){
    uint8_t newWindow = this->contentionWindow - 1;

    if (newWindow >= this->minimum){
      this->contentionWindow = newWindow;
    }
  }

  
  void increaseContentionWindow(){
    uint8_t newWindow = this->contentionWindow << 1;

    if (newWindow <= this->maximum){
      this->contentionWindow = newWindow;
    }
  }

  public:
     MILD_BACKOFF(){
      this->minimum = 15;
      this->contentionWindow = 15;
      this->maximum = 1023;
     }

};


class LINEAR_BACKOFF: public CONTENTION_BACKOFF{
  void reduceContentionWindow(){
    uint8_t newWindow = this->contentionWindow - 1;

    if (newWindow >= this->minimum){
      this->contentionWindow = newWindow;
    }
  }

  
  void increaseContentionWindow(){
    uint8_t newWindow = this->contentionWindow + 1;

    if (newWindow <= this->maximum){
      this->contentionWindow = newWindow;
    }
  }

  public:
     LINEAR_BACKOFF(){
      this->minimum = 15;
      this->contentionWindow = 15;
      this->maximum = 1023;
     }
};

CC1101 radio;

byte syncWord[2] = {199, 10};
volatile bool packetWaiting;

uint8_t myMacAddress[MAC_ADDRESS_SIZE];

// Packet and frame used by the sender.
TRAFFIC_GEN * trf_gen;

CSMA_CONTROL * csma_control;

/*
 * indicates to the code if packet should use automatic response or if is expecting data and code will linearly deal with it
*/
bool automaticResponse = true;

TaskHandle_t receiveHandle = NULL;
void messageReceived() {
  if (automaticResponse){
    portYIELD_FROM_ISR(xTaskResumeFromISR(receiveHandle));
  }
  else{
    packetWaiting = true;//self started communication; should be an answer, and code should be able to answer linearly
  }
}

/**
 * no setup needed, exists to be modified when receiving
*/
CCPACKET packet_to_receive;
ieeeFrame * receiveFrame = (ieeeFrame *) packet_to_receive.data;

//can be either cts or ack
CCPACKET answer_packet; 
ieeeFrame * answerFrame = (ieeeFrame *) answer_packet.data;

/**
 * Task will handle any communication started by another party
 */
void receiveAndAnswerTask(void* unused_param){

  while(true){

    vTaskSuspend(receiveHandle);  //suspend self; done on activation and after each receive

    //Serial.println("R&A awake");
    if(!receiver()){
      continue;
    }

    bool destIsMe = true;
    for( int i = 0; i<MAC_ADDRESS_SIZE; i++ ){
      if( receiveFrame->addr_dest[i] != myMacAddress[i] ){
        destIsMe = false;
        break;
      }
    }

    if( !destIsMe ){
      detachInterrupt(CC1101_GDO0);
      delayMicroseconds(receiveFrame->duration);
      attachInterrupt(CC1101_GDO0, messageReceived, RISING);
      continue;
    }

    //Set destination address
    memcpy( answerFrame->addr_dest, receiveFrame->addr_src, MAC_ADDRESS_SIZE );

    //Serial.println("Received frame on response task");
    
    if(PACKET_IS_DATA(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_ACK(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - radio.transmittionTime(packet_to_receive);
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        Serial.println("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);


      Serial.println("SENT ACK");
    }
    else if (PACKET_IS_RTS(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_CTS(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - radio.transmittionTime(packet_to_receive); //receive time could be saved, rts should always have same size and therefore time
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        Serial.println("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);

      Serial.println("SENT CTS");
    }
    else{
      Serial.printf("Response task, frame control %d; or 0x%x (hex) was not recognized\n", (uint) receiveFrame->frame_control[0], receiveFrame->frame_control[0]);

      /*
      Serial.print(F("packet: len "));
      Serial.println(packet_to_receive.length);
      Serial.println(F("data: "));
      Serial.printf("src: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            receiveFrame->addr_dest[0], 
            receiveFrame->addr_dest[1], 
            receiveFrame->addr_dest[2], 
            receiveFrame->addr_dest[3], 
            receiveFrame->addr_dest[4], 
            receiveFrame->addr_dest[5]);
      Serial.printf("frame_control[1]; %d; duration[0]: %d\n", (int) receiveFrame->frame_control[1], (int) receiveFrame->duration[0]);
      Serial.print(F("Payload: "));
      Serial.println((const char *) receiveFrame->payload);
      */
    }

  }
      
}

bool checkChannel(){  
  return radio.cca();
}


/**
 * calculates amount of time needed at beggining of transmittion
 * Used for NAV
 * 
 * Assumes as the rest of the code data can be sent in one burst
 * 
 * @param data_packet data packet to be sent
 */
uint16_t durationCalculation(CCPACKET data_packet){
  return radio.transmittionTime(data_packet) + (2 * radio.transmittionTime(answer_packet)) + 3 * SIFS; // 3 SIFS, ack and cts
}

/**
 * probably not a good idea to use either receive/answer variable packets due to misuse could cause
 * packet change from interruption, but could be done
*/
CCPACKET rts_packet;
ieeeFrame * rtsFrame = (ieeeFrame *) rts_packet.data;

/**
 * semaphore used either to gain access to radio mac protocol
 * wether to read or write
*/
SemaphoreHandle_t xSemaphore = xSemaphoreCreateBinary();

/**
  Test task. Should instead receive info, probably through a queue

  Uses mutex, as doing something like substituting CSMA_CONTROL when in CSMA_CONTROL
  function would probably cause big issues
*/
void coreZeroInitiator(void* unused_param){

  bool flip_flop = true; // to decide change each time, test purposes

  while(true){
    Serial.println(F("\nSecond core awake"));

    //taskENTER_CRITICAL(&communicationMux);
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    Serial.println("\n\nGOT EXCLUSIVITY\n\n");

#ifdef CACHE_CHANGE_IN_USE
    *cacheRegisterMutex = DPORT_CACHE_MUX_MODE_REG_SWITCH_VAL;
#endif

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setIdleState(); // to avoid rx overflow
    Serial.println("Set to idle state");
#endif

    delete csma_control;

    Serial.println("DELETED");

    flip_flop = !flip_flop;

    csma_control = (flip_flop) ? new CSMA_CONTROL(&checkChannel, new MILD_BACKOFF()) : new CSMA_CONTROL(&checkChannel, new LINEAR_BACKOFF());

    Serial.println("new csma_control");

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setRxState();
    Serial.println("ON RX STATE");
#endif
    
#ifdef CACHE_CHANGE_IN_USE
    *cacheRegisterMutex = DPORT_CACHE_MUX_MODE_REG_DEF_VAL;
#endif
    //taskEXIT_CRITICAL(&communicationMux);
    xSemaphoreGive(xSemaphore);

    Serial.println(F("CHANGED, add sleep or delay\n"));

    delay(2000);
    
  }

}



void setup() {

    // Serial communication for debug
    Serial.begin(57600);

    // Wifi, for getting the MAC address.
    //WiFi.mode(WIFI_STA); -- Each ndde will be it's own Access Point, so better to use AP
    //  WiFi.mode(WIFI_AP);
    //WiFi.STA.begin(); -- No need to start WiFi to get MAC
    //esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
    esp_wifi_get_mac(WIFI_IF_AP, myMacAddress);

    delay(1000);

    Serial.printf("My MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
      myMacAddress[0], 
      myMacAddress[1], 
      myMacAddress[2], 
      myMacAddress[3], 
      myMacAddress[4], 
      myMacAddress[5]);

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

    //Will crash and core dump on Core 0:
    //wifi_com_start(myMacAddress);

    //needs to be given once at first?
    //there shoud be a way to initialize xSemaphore withou this issue, TODO
    xSemaphoreGive(xSemaphore);
    
    
    xTaskCreatePinnedToCore(
      &coreZeroInitiator,
      "Communicate with the app",
      10000,
      &myMacAddress,
      0,
      NULL,
      0 //putting wifi config on it's own core
    );
    
    

    delay(2000);

    csma_control = new CSMA_CONTROL(&checkChannel, new MILD_BACKOFF());


    //answer packet definition
    answer_packet.length = sizeof(ieeeFrame);
    memcpy(answerFrame->addr_src, myMacAddress, MAC_ADDRESS_SIZE);

    //rts packet definition
    uint16_t data_length = sizeof(ieeeFrame) + 1000;
    rts_packet.length = data_length; //has data_length to calculate duration
    uint16_t rts_duration = durationCalculation(rts_packet);
    rts_packet.length = sizeof(ieeeFrame); //right length for rts
    memcpy(rtsFrame->addr_src, myMacAddress, MAC_ADDRESS_SIZE);
    PACKET_TO_RTS(rtsFrame);
    
    //4C:11:AE:64:D1:8D
    uint8_t dstMacAddress[6] = {0x4C, 0x11, 0xAE, 0x64, 0xD1, 0x8D};
    memcpy(rtsFrame->addr_dest, dstMacAddress, MAC_ADDRESS_SIZE);

    trf_gen = new TRAFFIC_GEN(&sender, myMacAddress, dstMacAddress, rts_duration, data_length);
    trf_gen->setTime(TRF_GEN_GAUSS, 6000);
    
    
    xTaskCreatePinnedToCore(
      &receiveAndAnswerTask,
      "receive and send acknowledge",
      10000, //no thought used to decide size
      NULL,
      10, // believe it should be bigger than traffic generation
      &receiveHandle,
      1 //putting related to cc1101 on same core
    );
    

    attachInterrupt(CC1101_GDO0, messageReceived, RISING);

    radio.setRxState();
}

TaskHandle_t generatorHandle = NULL;

void generatorTask(void* unusedParam){
  trf_gen->init();//is already a loop. Maybe change to add init restart on isRunning == false?
}

void loop(){
    if(!trf_gen->isRunning()){
        Serial.print(F("Initiating traffic..., loop has priority ")); 
        Serial.println(uxTaskPriorityGet(NULL));
        xTaskCreatePinnedToCore(
          &generatorTask,     // Function to execute
          "traffic generator",   // Name of the task
          10000,      // Stack size
          NULL,      // Task parameters
          2,         // Priority
          &generatorHandle,      // Task handle
          1          // Core 1
        );
    }

    //Handle wifi config here? Move xTask to setup. IS this already core 0 by default?
}

/**
 * requires radio initialization, csma control instance, receive task creation
 * Contains retry logic for now at least
 */
void sender(CCPACKET packet_to_send) { 

  Serial.println(F("TO SEND"));

  uint8_t retryCount = 0;

  unsigned long start_time; //used for both rts and data wait

  //taskENTER_CRITICAL(&communicationMux);
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  //Label
  send:

  //should be able to answer while waiting for turn, so it cannot be deactivated

  if(retryCount == 10){
    Serial.println(F("GIVING UP after retry limit reached"));
    //taskEXIT_CRITICAL(&communicationMux);
    xSemaphoreGive(xSemaphore);
    return;
  }
  //Serial.println(radio.readStatusReg(CC1101_MARCSTATE));
  csma_control->waitForTurn();

  //Serial.println(F("OUT OF CSMA_WAIT"));

  //Serial.println(radio.readStatusReg(CC1101_MARCSTATE));
  ///*
  detachInterrupt(CC1101_GDO0);

  Serial.printf("Dst MAC for RTS: %02X:%02X:%02X:%02X:%02X:%02X\n", 
      rtsFrame->addr_dest[0], 
      rtsFrame->addr_dest[1], 
      rtsFrame->addr_dest[2], 
      rtsFrame->addr_dest[3], 
      rtsFrame->addr_dest[4], 
      rtsFrame->addr_dest[5]);
  radio.sendData(rts_packet);
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  automaticResponse = false;

  start_time = micros();

  while(!packetWaiting){
    //sifs wait
    if(micros() - start_time >= SIFS){
      Serial.println(F("WAIT FOR CTS FAILED"));
      retryCount += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }

  }

  //checks if is ok and is an ack ack
  if(receiver() && !PACKET_IS_CTS(receiveFrame)){
    Serial.println(F("answer NOT a CTS"));
    retryCount += 1;
    csma_control->ackReceived(false);
    goto send;
    
  }

  //*/

  detachInterrupt(CC1101_GDO0);
  radio.sendData(packet_to_send);
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  automaticResponse = false;
  start_time = micros();

  while(!packetWaiting){
    //sifs wait
    if(micros() - start_time >= SIFS){
      Serial.println(F("No ack"));
      retryCount += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }

  }

  automaticResponse = true;
  
  //checks if is ok and is an ack ack
  if(receiver() && !PACKET_IS_ACK(receiveFrame)){
    Serial.println("answer is NOT an ACK");
    retryCount += 1;
    csma_control->ackReceived(false);
    goto send;
    
  }

  
  csma_control->ackReceived(true);
  //to eventually add sucess to register, so it can be accessed by the esp wifi

  //taskEXIT_CRITICAL(&communicationMux);
  xSemaphoreGive(xSemaphore);

  Serial.println(F("Complete success"));
}

/**
 * Directly affects 'packet_to_receive' global var
 * returns CCPACKET.crc_ok
 */
bool receiver() {

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  radio.receiveData(&packet_to_receive);
  
  //only necessary if is not handled by task; probably shoul be placed somewhere else; TODO
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  return packet_to_receive.crc_ok;

}
