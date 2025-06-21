#include <Arduino.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"
#include "src/wifi_config/wifi_config.h"

/**
 * to uncomment if answer task logic changes with mac protocol parameter change
*/
#define ANSWER_TASK_CHANGES_WITH_PARAMETERS "Irrelevant value"

#define ANSWER_TASK_PRIORITY 10
#define TRAFFIC_GENERATOR_PRIORITY 5
#define PARAMETER_CHANGE_PRIORITY 2 // smaller than traffic generation
#define LOOP_PRIORITY 1

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
 * 
 * Is able to answer either to rts or data packets
 * 
 * Implements NAV by doing an active wait where it deactivates packet receiving capabilities
 */
void receiveAndAnswerTask(void* unused_param){

  while(true){

    vTaskSuspend(receiveHandle);  //suspend self; done on activation and after each receive

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
      uint16_t wait_time =  receiveFrame->duration;
      unsigned long wait_start = micros();

      radio.setIdleState();

      while (micros() - wait_start <= wait_time);

      radio.setRxState();
      
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
 * list of backoff protocols from which to choose
*/
enum BACKOFF_PROTOCOLS{
  MILD,
  LINEAR
};

/**
 * parameter collection for trafficGenerator
*/
struct trafficGeneratorParameters{
  /**
   * flag to communicate if values ware actually given
  */
  bool used;

  uint8_t time_mode;

  uint16_t waiting_time;
};


struct csmaControlParameters{
  /**
   * flag to communicate if values ware actually given
  */
  bool used;


  /*
   * wether backof is of MILD, Exponentional, or other.
   *
  */
  BACKOFF_PROTOCOLS backoff_protocol;
};

/**
 * Contains all possible data used to reconfigure mac protocol
*/
struct macProtocolParameters{
  trafficGeneratorParameters traf_gen_params;

  csmaControlParameters csma_contrl_params;
};

/**
 * parameter queue
 *
 * size of queue as one, should not need to hold more than one value at once
*/
QueueHandle_t protocolParametersQueueHandle = xQueueCreate( 1, sizeof(macProtocolParameters) );








/**
  Test task. Should instead receive info, probably through a queue

  Uses mutex, as doing something like substituting CSMA_CONTROL when in CSMA_CONTROL
  function would probably cause big issues
*/
void coreZeroInitiator(void* unused_param){

  bool flip_flop = true; // to decide change each time, test purposes

  uint8_t params_buffer[sizeof(macProtocolParameters)];

  macProtocolParameters *params = (macProtocolParameters*) params_buffer;
  params->csma_contrl_params.used = true;

  while(true){
    Serial.println(F("\nSecond core awake"));

    flip_flop = !flip_flop;

    if(flip_flop){
      params->csma_contrl_params.backoff_protocol = MILD;
    } else{
      params->csma_contrl_params.backoff_protocol = LINEAR;
    }

    xQueueSend(protocolParametersQueueHandle, (const void*) params_buffer, portMAX_DELAY);
    
    Serial.println(F("Given values, does not wait for other task to change values\n"));

    delay(2000);
    
  }

}


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
 * TODO consider if parameter receive should happen either by  function params and task is recreated
 * each time or it should receive them from a queue
 *
*/
void changeParametersTask(void* unusedParam){

  //Implementing queue solution for now

  Serial.println(F("Created change parameters task"));

  uint8_t params_buffer[sizeof(macProtocolParameters)];

  macProtocolParameters *params = (macProtocolParameters*) params_buffer;

  while(true){

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setIdleState(); // to avoid rx overflow
    Serial.println("Set to idle state, avoiding answer task being activated");
#endif

    /**
     * check if portMAX_DELAY wait is forever
    */
    if(xQueueReceive(protocolParametersQueueHandle, params_buffer, portMAX_DELAY) == pdFALSE){
      continue;
    }

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    Serial.println("\n\nCHANGE PARAMETERS GOT EXCLUSIVITY\n\n");

    if(params->csma_contrl_params.used){
      delete csma_control;
      switch(params->csma_contrl_params.backoff_protocol){
        case MILD:
          csma_control = new CSMA_CONTROL(&checkChannel, new MILD_BACKOFF());
          Serial.println("Changed csma_control to MILD_BACKOFF");
          break;

        case LINEAR:
          csma_control = new CSMA_CONTROL(&checkChannel, new LINEAR_BACKOFF());
          Serial.println("Changed csma_control to LINEAR_BACKOFF");
          break;
      }
    }


    if(params->traf_gen_params.used){
      trf_gen->setTime(params->traf_gen_params.time_mode, params->traf_gen_params.waiting_time);
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setRxState();
    Serial.println("On rx state, allowing answer task activating");
#endif

    xSemaphoreGive(xSemaphore);
    Serial.println(F("CHANGED, delay\n"));

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
    
    
    xTaskCreatePinnedToCore(
      &wifi_com_task,
      "App Comm",
      100000,
      &myMacAddress,
      0, //WARNING NEEDS TO BE 0, OTHERWISE WATCHDOG PROBLEMS
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
          TRAFFIC_GENERATOR_PRIORITY,         // Priority
          &generatorHandle,      // Task handle
          1          // Core 1
        );
    }

}

/**
 * requires radio initialization, csma control instance, receive task creation
 * Contains retry logic for now at least
 */
void sender(CCPACKET packet_to_send) { 

  Serial.println(F("TO SEND"));

  uint8_t retryCount = 0;

  unsigned long start_time; //used for both rts and data wait

  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  //Label
  send:

  //should be able to answer while waiting for turn, so it cannot be deactivated

  if(retryCount == 10){
    Serial.println(F("GIVING UP after retry limit reached"));
    xSemaphoreGive(xSemaphore);
    return;
  }
  //Serial.println(radio.readStatusReg(CC1101_MARCSTATE));
  csma_control->waitForTurn();

  //Serial.println(F("OUT OF CSMA_WAIT"));

  //Serial.println(radio.readStatusReg(CC1101_MARCSTATE));
  ///*
  detachInterrupt(CC1101_GDO0);
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
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    configGENERATE_RUN_TIME_STATS();

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
  
  //only necessary if is handled by traffic task; probably shoul be placed somewhere else; TODO
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  return packet_to_receive.crc_ok;

}
