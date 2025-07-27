#include <Arduino.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"
#include "src/wifi_config/wifi_config.h"
#include "src/param_data/param_data.h"
#include "src/mac_data/mac_data.h"

/**
 * to uncomment if answer task logic changes with mac protocol parameter change
*/
#define ANSWER_TASK_CHANGES_WITH_PARAMETERS "Irrelevant value"

#define ANSWER_TASK_PRIORITY 10
#define TRAFFIC_GENERATOR_PRIORITY 5
#define PARAMETER_CHANGE_PRIORITY 2 // smaller than traffic generation
/**
 * If not 0 watchdog from core 0 is activated, since it contains blocking function
*/
#define WIFI_TASK_PRIORITY 0
#define LOOP_PRIORITY 1

/**
 * When defined, code is on debug mode, and prints values with the defines
 *
 * If not defined code should not do prints
*/
//#define MONITOR_DEBUG_MODE //Does not need a value

#ifdef MONITOR_DEBUG_MODE
#define PRINTLN(string) Serial.println(F(string))
#define PRINT(string) Serial.print(F(string))
#define PRINTLN_VALUE(value) Serial.println(value)
#define PRINTLN_MAC(value) Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n", value[0], value[1], value[2], value[3], value[4], value[5])
#else
#define PRINTLN(void) 
#define PRINT(void) 
#define PRINTLN_VALUE(void)
#define PRINTLN_MAC(void) 
#endif


#define DEFAULT_BACKOFF_ALGORITHM CONSTANT
/**
 * content does not include size for frame control bits (ieeeFrame)
*/
#define DEFAULT_FRAME_CONTENT_SIZE 1000

/*
 * Array value declaration. Ensure array is big enough for MAC address
*/
#define DEFAULT_MAC_ADDRESS {0x4C, 0x11, 0xAE, 0x64, 0xD1, 0x8D}



#define DEFAULT_TIME_INTERVAL_MODE TRF_GEN_GAUSS

#define DEFAULT_TIME_INTERVAL 0

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
 * Checks if destintation of message is for the node. Assumes read packet is packet_to_receive
 * 
 * Return true if dest is for the node, false oterwise
 */
bool destIsMe(){
  for( int i = 0; i<MAC_ADDRESS_SIZE; i++ ){
      if( receiveFrame->addr_dest[i] != myMacAddress[i] ){
        return false;
      }
    }
  return true;
}

/**
 * Task will handle any communication started by another party
 * 
 * Is able to answer either to rts or data packets
 * 
 * Implements NAV by doing an active wait where it deactivates packet receiving capabilities
 */
void receiveAndAnswerTask(void* unused_param){

  while(true){

    vTaskSuspend(NULL);  //suspend self; done on activation and after each receive

    if(!receiver()){
      continue;
    }
    
    PRINT("DST: ");
    PRINTLN_MAC(receiveFrame->addr_dest);

    //Set destination address
    memcpy( answerFrame->addr_dest, receiveFrame->addr_src, MAC_ADDRESS_SIZE );

    //PRINTLN("Received frame on response task");
    
    if(PACKET_IS_DATA(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_ACK(answerFrame);
      answerFrame->duration = 0; // no more to send after this, since fragmentation is not supported
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);


      PRINTLN("SENT ACK");
    }
    else if (PACKET_IS_RTS(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_CTS(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - radio.transmittionTime(packet_to_receive); 
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);

      PRINTLN("SENT CTS");
    }
    else{
      PRINT("Response task, frame control not recognized, with value: ");
      PRINTLN_VALUE(receiveFrame->frame_control[0]);
    }

  }
      
}

bool checkChannel(){  
  return radio.cca();
}

/**
 * calculates amount of time needed at beggining of transmittion
 * Used for rts NAV
 * 
 * Assumes as the rest of the code data can be sent in one burst
 * 
 * @param data_length length of content from data packet to be sent
 */
uint16_t durationCalculation(unsigned short data_length){
  CCPACKET data_packet;
  data_packet.length = sizeof(ieeeFrame) + data_length;
  return radio.transmittionTime(data_packet) + (2 * radio.transmittionTime(answer_packet)) + 3 * SIFS; // 3 SIFS, ack and cts
}

/**
 * calculation of duration filed for NAV found on the data
*/
uint16_t dataDurationCalculation(){
  return radio.transmittionTime(answer_packet) + SIFS; // 1 SIFS + ack time
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
SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();

/**
 * Task to change parameters of protocol, running on the same core to ensure cache is correct
 * 
 * Not responsible of collecting parameters, receives them
 *
 * Task continuously receives parameters from queue
*/
void changeParametersTask(void* unusedParam){

  //Implementing queue solution for now

  PRINTLN("Created change parameters task");

  uint8_t params_buffer[sizeof(macProtocolParameters)];

  macProtocolParameters *params = (macProtocolParameters*) params_buffer;

  while(true){

    /**
     * check if portMAX_DELAY wait is forever
    */
    if(xQueueReceive(protocolParametersQueueHandle, params_buffer, portMAX_DELAY) == pdFALSE){
      continue;
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setIdleState(); // to avoid rx overflow
    PRINTLN("Set to idle state, avoiding answer task being activated");
#endif

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    PRINTLN("\n\nCHANGE PARAMETERS GOT EXCLUSIVITY\n\n");

    mac_data.retries = 0; mac_data.failures = 0; mac_data.successes = 0; mac_data.startTime = millis();

    if(params->csma_contrl_params.used){
      delete csma_control;
      csma_control = new CSMA_CONTROL(&checkChannel, getBackoffProtocol(params->csma_contrl_params.backoff_protocol));
    }


    if(params->traf_gen_time.used){
      trf_gen->setTime(params->traf_gen_time.time_mode, params->traf_gen_time.waiting_time);
    }

    if(params->traf_gen_addr.used){
      PRINT("CHANGE PARAMS TASK NEW DEST MAC: "); PRINTLN_MAC(params->traf_gen_addr.address);
      trf_gen->setDestAddress(params->traf_gen_addr.address);
      memcpy(rtsFrame->addr_dest, params->traf_gen_addr.address, MAC_ADDRESS_SIZE);
    }

    if(params->traf_gen_data.used){
      Serial.println("Changing message");

      rtsFrame->duration = durationCalculation(params->traf_gen_data.message_length);
      trf_gen->setMessage(
        params->traf_gen_data.message, 
        params->traf_gen_data.message_length
      );

      //TODO find better solution than malloc and free if possible
      free(params->traf_gen_data.message);
      
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setRxState();
    PRINTLN("On rx state, allowing answer task activating");
#endif

    xSemaphoreGive(xSemaphore);
    PRINTLN("CHANGED, delay\n");

  }

}

void setup() {

    // Serial communication for debug
    Serial.begin(57600);

    // Wifi, for getting the MAC address.
    WiFi.mode(WIFI_AP);
    //WiFi.STA.begin(); -- No need to start WiFi to get MAC
    //esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
    esp_wifi_get_mac(WIFI_IF_AP, myMacAddress);

    delay(1000);

  #ifndef MONITOR_DEBUG_MODE
    Serial.printf("My MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
      myMacAddress[0], 
      myMacAddress[1], 
      myMacAddress[2], 
      myMacAddress[3], 
      myMacAddress[4], 
      myMacAddress[5]);
  #endif

    // Initialize the CC1101 radio
    radio.init();
    radio.setSyncWord(syncWord);
    radio.setCarrierFreq(CFREQ_433);
    radio.disableAddressCheck();
    radio.setTxPowerAmp(PA_LowPower);

    delay(1000);

    // Print some debug info
    PRINT("CC1101_PARTNUM ");
    PRINTLN_VALUE(radio.readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER));
    PRINT("CC1101_VERSION ");
    PRINTLN_VALUE(radio.readReg(CC1101_VERSION, CC1101_STATUS_REGISTER));
    PRINT("CC1101_MARCSTATE ");
    PRINTLN_VALUE(radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);

    PRINTLN_VALUE("CC1101 radio initialized.");    
    
    
    xTaskCreatePinnedToCore(
      &wifi_com_task,
      "App Comm",
      100000,
      &myMacAddress,
      WIFI_TASK_PRIORITY,
      NULL,
      0 //putting wifi config on it's own core
    );

    delay(2000);

    csma_control = new CSMA_CONTROL(&checkChannel, getBackoffProtocol(DEFAULT_BACKOFF_ALGORITHM));

    //answer packet definition
    answer_packet.length = sizeof(ieeeFrame);
    memcpy(answerFrame->addr_src, myMacAddress, MAC_ADDRESS_SIZE);

    //rts packet definition
    rts_packet.length = sizeof(ieeeFrame); //right length for rts
    memcpy(rtsFrame->addr_src, myMacAddress, MAC_ADDRESS_SIZE);
    PACKET_TO_RTS(rtsFrame);
    rtsFrame->duration = durationCalculation(DEFAULT_FRAME_CONTENT_SIZE);
    
    //4C:11:AE:64:D1:8D
    uint8_t dstMacAddress[6] = DEFAULT_MAC_ADDRESS;
    memcpy(rtsFrame->addr_dest, dstMacAddress, MAC_ADDRESS_SIZE);

    trf_gen = new TRAFFIC_GEN(&sender, myMacAddress, dstMacAddress, dataDurationCalculation(), sizeof(ieeeFrame) + DEFAULT_FRAME_CONTENT_SIZE);
    trf_gen->setTime(DEFAULT_TIME_INTERVAL_MODE, DEFAULT_TIME_INTERVAL);

    //char def_msg[] = "Default Message";
    //trf_gen->setMessage(def_msg, strlen(def_msg));
    
    
    xTaskCreatePinnedToCore(
      &receiveAndAnswerTask,
      "receive and send acknowledge",
      10000, //no thought used to decide size
      NULL,
      ANSWER_TASK_PRIORITY,
      &receiveHandle,
      1 //putting related to cc1101 on same core
    );

    xTaskCreatePinnedToCore(
      &changeParametersTask,
      "change parameters task",
      5000, //no thought used to decide size
      NULL,
      PARAMETER_CHANGE_PRIORITY, 
      NULL,
      1 //putting related to cc1101 on same core
    );
    

    mac_data.startTime = millis();

    attachInterrupt(CC1101_GDO0, messageReceived, RISING);

    radio.setRxState();
    
}

TaskHandle_t generatorHandle = NULL;

void generatorTask(void* unusedParam){
  trf_gen->init();//is already a loop. Maybe change to add init restart on isRunning == false?
}

void loop(){
    /*
    if(!trf_gen->isRunning()){
        PRINT("Initiating traffic..., loop has priority "); 
        PRINTLN_VALUE(uxTaskPriorityGet(NULL));
        xTaskCreatePinnedToCore(
          &generatorTask,     // Function to execute
          "traffic generator",   // Name of the task
          10000,      // Stack size
          NULL,      // Task parameters
          TRAFFIC_GENERATOR_PRIORITY,         // Priority
          &generatorHandle,      // Task handle
          1          // Core 1
        );
    }
    */

    int random = (int) esp_random();

    if(random < 0) random = -random;

    int ranged_rand = random / (RAND_MAX / (15 + 1) + 1);
    //int random = (int) esp_random();
    //int ranged_rand = random / (RAND_MAX / (15 + 1) + 1);
    Serial.printf("Raw random: %d, limited rand from 0..15: %d \n", random, ranged_rand);


    delay(400);
}

/**
 * requires radio initialization, csma control instance, receive task creation
 * Contains retry logic for now at least
 */
void sender(CCPACKET packet_to_send) { 

  PRINTLN("TO SEND");

  uint8_t retryCount = 0;

  unsigned long start_time; //used for both rts and data wait

  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  //Label
  send:

  //should be able to answer while waiting for turn, so it cannot be deactivated

  if(retryCount == 10){
    PRINTLN("GIVING UP after retry limit reached");
    xSemaphoreGive(xSemaphore);
    mac_data.failures += 1;
    return;
  }
  //PRINTLN_VALUE(radio.readStatusReg(CC1101_MARCSTATE));
  csma_control->waitForTurn();

  ///*
  detachInterrupt(CC1101_GDO0);
  radio.sendData(rts_packet);
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  automaticResponse = false;

  start_time = micros();

  while(!packetWaiting){
    //sifs wait
    if(micros() - start_time >= SIFS){
      PRINTLN("WAIT FOR CTS FAILED");
      retryCount += 1; mac_data.retries += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }

  }

  //checks if is ok and is an ack ack
  if(receiver() && !PACKET_IS_CTS(receiveFrame)){
    PRINTLN("answer NOT a CTS");
    retryCount += 1; mac_data.retries += 1;
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
      PRINTLN("No ack");
      retryCount += 1; mac_data.retries += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }

  }

  automaticResponse = true;
  
  //checks if is ok and is an ack ack
  if(receiver() && !PACKET_IS_ACK(receiveFrame)){
    PRINTLN("answer is NOT an ACK");
    retryCount += 1; mac_data.retries += 1;
    csma_control->ackReceived(false);
    goto send;
    
  }

  
  csma_control->ackReceived(true);

  xSemaphoreGive(xSemaphore);
  mac_data.successes += 1;

  PRINTLN("Complete success");
}

/**
 * Directly affects 'packet_to_receive' global var, implements NAV
 * returns true if crc is ok and dest is self else returns false and waits nav if needed
 */
bool receiver() {

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  radio.receiveData(&packet_to_receive);
  
  //only necessary if is handled by traffic task; probably shoul be placed somewhere else; TODO
  packetWaiting = false;

  if(!destIsMe()){

    if (receiveFrame->duration == 0){
      return false;
    }
    
    uint16_t wait_time =  receiveFrame->duration;
    unsigned long wait_start = micros();

    radio.setIdleState();

    while (micros() - wait_start <= wait_time);

    radio.setRxState();

    return false;
  }

  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  return packet_to_receive.crc_ok;

}
