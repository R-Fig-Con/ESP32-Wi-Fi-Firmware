#include <Arduino.h>
#include <WebServer.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"
#include "src/wifi_config/wifi_config.h"
#include "src/param_data/param_data.h"

/**
  * cache Test
  * 
  * Warning using http to unblock print does not seem to work, use serial monitor write 
*/

/**
 * to uncomment if answer task logic changes with mac protocol parameter change
*/
#define ANSWER_TASK_CHANGES_WITH_PARAMETERS "Irrelevant value"

#define ANSWER_TASK_PRIORITY 10
#define TRAFFIC_GENERATOR_PRIORITY 5
#define PARAMETER_CHANGE_PRIORITY 3 // smaller than traffic generation
/**
 * If not 0 watchdog from core 0 is activated, since it contains blocking function
*/
#define WIFI_TASK_PRIORITY 0
/**
 * Not used as priority of loop is automatic
 *
 * done as documentation, comparing to other task priorities
*/
#define LOOP_PRIORITY 1

/**
 * When defined, code is on debug mode, and prints values with the defines
 *
 * If not defined code should not do prints
*/
//#define MONITOR_DEBUG_MODE "useless value"

#ifdef MONITOR_DEBUG_MODE
  #define PRINTLN(string) Serial.println(F(string))
  #define PRINT(string) Serial.print(F(string))
  #define PRINTLN_VALUE(value) Serial.println(value)
#else
  #define PRINTLN(string) 
  #define PRINT(string) 
  #define PRINTLN_VALUE(string) 
#endif

/**
 * If defined, it unblocks through a http request to http://IP_ADDRESS/list; does not seem to work
 * 
 * If not defined unblocks through serial write. Click enter directly on serial monitor write box
 * of arduino
*/
//#define UNBLOCK_PRINT_WITH_HTTP


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

  int id(){
    return MILD;
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

  int id(){
    return LINEAR;
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

      // // Serial.println("MAC NOT FOR ME; TO DELETE PRINT");

      while (micros() - wait_start <= wait_time);

      radio.setRxState();
      
      continue;
    }

    //Set destination address
    memcpy( answerFrame->addr_dest, receiveFrame->addr_src, MAC_ADDRESS_SIZE );

    //PRINTLN("Received frame on response task");
    
    if(PACKET_IS_DATA(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_ACK(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - radio.transmittionTime(packet_to_receive);
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);


      PRINTLN("SENT ACK");
      // // Serial.println("SENT ACK DELETE");
    }
    else if (PACKET_IS_RTS(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_CTS(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - radio.transmittionTime(packet_to_receive); //receive time could be saved, rts should always have same size and therefore time
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);

      PRINTLN("SENT CTS");
      // // Serial.println("SENT CTS DELETE");
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
SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();

/**
  TODO as test task to delete or comment

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
    PRINTLN("\nSecond core awake");

    flip_flop = !flip_flop;

    if(flip_flop){
      params->csma_contrl_params.backoff_protocol = MILD;
    } else{
      params->csma_contrl_params.backoff_protocol = LINEAR;
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setIdleState(); // to avoid rx overflow
    PRINTLN("Set to idle state, avoiding answer task being activated");
#endif

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    PRINTLN("\n\nCHANGE PARAMETERS GOT EXCLUSIVITY\n\n");

    if(params->csma_contrl_params.used){
      delete csma_control;
      switch(params->csma_contrl_params.backoff_protocol){
        case MILD:
          csma_control = new CSMA_CONTROL(&checkChannel, new MILD_BACKOFF());
          PRINTLN("Changed csma_control to MILD_BACKOFF");
          Serial.println(F("CHANGED TO MILD; DELETE"));
          break;

        case LINEAR:
          csma_control = new CSMA_CONTROL(&checkChannel, new LINEAR_BACKOFF());
          PRINTLN("Changed csma_control to LINEAR_BACKOFF");
          Serial.println(F("CHANGED TO LINEAR; DELETE"));
          break;
      }
    }

    if(params->traf_gen_params.used){
      trf_gen->setTime(params->traf_gen_params.time_mode, params->traf_gen_params.waiting_time);
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setRxState();
    PRINTLN("On rx state, allowing answer task activating");
#endif

    xSemaphoreGive(xSemaphore);
    PRINTLN("CHANGED, delay\n");

    delay(2000);
    
  }

}

int csma_version[2000];
int csma_version_size = 0;

/**
 * task to check csma_version history in core 1
 *
 * to use when changing on core 0, for task reasons
 *
 * priority should be lower than answer and traffic as to not block
*/
void print_csma_version(void* unused){
  
  Serial.println("printer init");

  while(true){

#ifdef UNBLOCK_PRINT_WITH_HTTP
    vTaskSuspend(NULL);
#else
    while(Serial.available() <= 0);
    Serial.read();    
#endif

    Serial.println("printer unblocked");

    for(int i = 0; i < csma_version_size; i++){
      Serial.printf("%d;\t", csma_version[csma_version_size]);
    }

    Serial.println(F("\n\n"));

  }

}



TaskHandle_t printerHandle = NULL;

#ifdef UNBLOCK_PRINT_WITH_HTTP

  WebServer http_server(80);

  const char *ssid = "IBG"; // internet name
  const char *password = "b5bb300252"; // internet password, belive it can be null if it does not have one

  void unblock_print() {
    vTaskResume(printerHandle);
    http_server.send(200, "text/plain", "");
  }

  void http_server_task(void* unused){

    Serial.println("\n\nTo start SERVER");
    http_server.begin();
    Serial.println("Server started");

    while(true){
      http_server.handleClient();
    }

  }

#endif

void setup() {

    // Serial communication for debug
    Serial.begin(57600);

    delay(1000);
    // Wifi, for getting the MAC address.
    //WiFi.mode(WIFI_AP);

#ifdef UNBLOCK_PRINT_WITH_HTTP
    WiFi.begin(ssid, password);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.print("\tConnected! IP address: ");
    Serial.println(WiFi.localIP());

    http_server.on("/list", HTTP_GET, unblock_print);

#else
    WiFi.mode(WIFI_AP);
#endif

    //WiFi.STA.begin(); -- No need to start WiFi to get MAC
    //esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
    esp_wifi_get_mac(WIFI_IF_AP, myMacAddress);

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
      &coreZeroInitiator,
      "App Comm",
      100000,
      &myMacAddress,
      WIFI_TASK_PRIORITY,
      NULL,
      0 //putting wifi config on it's own core
    );


#ifdef UNBLOCK_PRINT_WITH_HTTP
    xTaskCreatePinnedToCore(
      &http_server_task,
      "Http server",
      100000,
      NULL,
      WIFI_TASK_PRIORITY,
      NULL,
      0 //putting wifi config on it's own core
    );
#endif

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
    //EDU: BOX 1C:69:20:30:DF:41
    uint8_t dstMacAddress[6] = {0x1C, 0x69, 0x20, 0x30, 0xDF, 0x41};
    memcpy(rtsFrame->addr_dest, dstMacAddress, MAC_ADDRESS_SIZE);

    Serial.printf("Dst MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
      rtsFrame->addr_dest[0], 
      rtsFrame->addr_dest[1], 
      rtsFrame->addr_dest[2], 
      rtsFrame->addr_dest[3], 
      rtsFrame->addr_dest[4], 
      rtsFrame->addr_dest[5]);

    trf_gen = new TRAFFIC_GEN(&sender, myMacAddress, dstMacAddress, rts_duration, data_length);
    trf_gen->setTime(TRF_GEN_CONST, 3000);
    
    
    xTaskCreatePinnedToCore(
      &receiveAndAnswerTask,
      "receive and send acknowledge",
      10000, //no thought used to decide size
      NULL,
      ANSWER_TASK_PRIORITY,
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
    //Serial.println("ON LOOP TO DELETE");
    if(!trf_gen->isRunning()){
        Serial.println("ON LOOP TO DELETE; traffic generation startup");
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

    if(printerHandle == NULL){
      Serial.println("ON LOOP TO DELETE; printer startup");
      xTaskCreatePinnedToCore(
        &print_csma_version,
        "print csma version",
        10000, //no thought used to decide size
        NULL,
        LOOP_PRIORITY + 1,
        &printerHandle,
        1 //putting related to cc1101 on same core
      );
    }

}

/**
 * requires radio initialization, csma control instance, receive task creation
 * Contains retry logic for now at least
 */
void sender(CCPACKET packet_to_send) { 

  PRINTLN("TO SEND");
  Serial.println("SEND TO DELETE; sending");

  uint8_t retryCount = 0;

  unsigned long start_time; //used for both rts and data wait


  if(csma_version_size < 2000){
    csma_version[csma_version_size] = csma_control->id();
    csma_version_size += 1;
  }


  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  //Label
  send:

  //should be able to answer while waiting for turn, so it cannot be deactivated

  if(retryCount == 10){
    PRINTLN("GIVING UP after retry limit reached");
    xSemaphoreGive(xSemaphore);
    return;
  }
  //PRINTLN_VALUE(radio.readStatusReg(CC1101_MARCSTATE));
  csma_control->waitForTurn();

  //PRINTLN("OUT OF CSMA_WAIT");

  //PRINTLN_VALUE(radio.readStatusReg(CC1101_MARCSTATE));
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
      // // Serial.println(F("Wait for cts; to delete and use define"));
      retryCount += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }

  }

  //checks if is ok and is an ack ack
  if(receiver() && !PACKET_IS_CTS(receiveFrame)){
    PRINTLN("answer NOT a CTS");
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
      PRINTLN("No ack");
      retryCount += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }

  }

  automaticResponse = true;
  
  //checks if is ok and is an ack ack
  if(receiver() && !PACKET_IS_ACK(receiveFrame)){
    PRINTLN("answer is NOT an ACK");
    retryCount += 1;
    csma_control->ackReceived(false);
    goto send;
    
  }

  
  csma_control->ackReceived(true);

  xSemaphoreGive(xSemaphore);

  PRINTLN_VALUE("Complete success");
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
