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
#include "src/utils/utils.h"

#define CCA_FROM_GDO2_PIN "Irrelevant value"

/**
 * to uncomment if answer task logic changes with mac protocol parameter change
 */
#define ANSWER_TASK_CHANGES_WITH_PARAMETERS "Irrelevant value"

#define ANSWER_TASK_PRIORITY 10
#define TRAFFIC_GENERATOR_PRIORITY 5

/**
 * Note (27 august) with current traffic generator, if delay value is 0 parameter
 * change task will not run when its priority is lower than the generator's
 */
#define PARAMETER_CHANGE_PRIORITY 6
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
// #define MONITOR_DEBUG_MODE //Does not need a value

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

#define DEFAULT_MAC_ADDRESS {0x1C, 0x69, 0x20, 0x30, 0xDF, 0x41}

#define DEFAULT_TIME_INTERVAL_MODE TRF_GEN_GAUSS

#define DEFAULT_TIME_INTERVAL 5000

byte syncWord[2] = {199, 10};

TaskHandle_t receiveHandle = NULL;
void messageReceived()
{
  if (send_protocol->give_automatic_response())
  {
    portYIELD_FROM_ISR(xTaskResumeFromISR(receiveHandle));
  }
  else
  {
    send_protocol->set_packet_flag();
  }
}

/**
 * Checks if destintation of message is for the node. Assumes read packet is packet_to_receive
 *
 * Return true if dest is for the node, false oterwise
 */
bool destIsMe()
{
  for (int i = 0; i < MAC_ADDRESS_SIZE; i++)
  {
    if (receiveFrame->addr_dest[i] != myMacAddress[i])
    {
      return false;
    }
  }
  return true;
}

bool checkChannel()
{
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
uint16_t durationCalculation(unsigned short data_length)
{
  CCPACKET data_packet;
  data_packet.length = sizeof(ieeeFrame) + data_length;
  return radio.transmittionTime(data_packet) + (2 * radio.transmittionTime(answer_packet)) + 3 * SIFS; // 3 SIFS, ack and cts
}

/**
 * calculation of duration filed for NAV found on the data
 */
uint16_t dataDurationCalculation()
{
  return radio.transmittionTime(answer_packet) + SIFS; // 1 SIFS + ack time
}

void setup()
{

  // Serial communication for debug
  Serial.begin(57600);

  // Wifi, for getting the MAC address.
  WiFi.mode(WIFI_STA);
  // WiFi.STA.begin(); -- No need to start WiFi to get MAC
  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);

  delay(1000);

  PRINT("My MAC:");
  PRINTLN_MAC(myMacAddress);

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

  send_protocol = new CSMA_CA();

  uint32_t mac_num = ((uint32_t)myMacAddress[2] << 24) | ((uint32_t)myMacAddress[3] << 16) |
                     ((uint32_t)myMacAddress[4] << 8) | ((uint32_t)myMacAddress[5]);
  srand(mac_num);

  xTaskCreatePinnedToCore(
      &wifi_com_task,
      "App Comm",
      100000,
      &myMacAddress,
      WIFI_TASK_PRIORITY,
      NULL,
      0 // putting wifi config on it's own core
  );

  delay(2000);

  csma_control = new CSMA_CONTROL(&checkChannel, getBackoffProtocol(DEFAULT_BACKOFF_ALGORITHM));

  // answer packet definition
  answer_packet.length = sizeof(ieeeFrame);
  memcpy(answerFrame->addr_src, myMacAddress, MAC_ADDRESS_SIZE);

  // rts packet definition
  rts_packet.length = sizeof(ieeeFrame); // right length for rts
  memcpy(rtsFrame->addr_src, myMacAddress, MAC_ADDRESS_SIZE);
  PACKET_TO_RTS(rtsFrame);
  rtsFrame->duration = durationCalculation(DEFAULT_FRAME_CONTENT_SIZE);

  // 4C:11:AE:64:D1:8D
  uint8_t dstMacAddress[6] = DEFAULT_MAC_ADDRESS;
  memcpy(rtsFrame->addr_dest, dstMacAddress, MAC_ADDRESS_SIZE);

  trf_gen = new TRAFFIC_GEN(send_protocol, myMacAddress, dstMacAddress, dataDurationCalculation(), sizeof(ieeeFrame) + DEFAULT_FRAME_CONTENT_SIZE);
  trf_gen->setTime(DEFAULT_TIME_INTERVAL_MODE, DEFAULT_TIME_INTERVAL);

  xTaskCreatePinnedToCore(
      &receiveAndAnswerTask,
      "receive and send acknowledge",
      10000, // no thought used to decide size
      NULL,
      ANSWER_TASK_PRIORITY,
      &receiveHandle,
      1 // putting related to cc1101 on same core
  );

  xTaskCreatePinnedToCore(
      &changeParametersTask,
      "change parameters task",
      5000, // no thought used to decide size
      NULL,
      PARAMETER_CHANGE_PRIORITY,
      NULL,
      1 // putting related to cc1101 on same core
  );

  mac_data.startTime = millis();

  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  radio.setRxState();
}

TaskHandle_t generatorHandle = NULL;

void generatorTask(void *unusedParam)
{
  trf_gen->init(); // is already a loop. Maybe change to add init restart on isRunning == false?
}

void loop()
{

  if (!trf_gen->isRunning())
  {
    PRINT("Initiating traffic..., loop has priority ");
    PRINTLN_VALUE(uxTaskPriorityGet(NULL));
    xTaskCreatePinnedToCore(
        &generatorTask,             // Function to execute
        "traffic generator",        // Name of the task
        10000,                      // Stack size
        NULL,                       // Task parameters
        TRAFFIC_GENERATOR_PRIORITY, // Priority
        &generatorHandle,           // Task handle
        1                           // Core 1
    );
  }
}

/**
 * Directly affects 'packet_to_receive' global var, implements NAV
 * returns true if crc is ok and dest is self else returns false and waits nav if needed
 *
 * Todo add sleep mode during nav instead of NAV. Should requires waiting until it gets idle
 * and use command strobes to change states. Check cc1101 datasheet
 */
bool receiver()
{

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  radio.receiveData(&packet_to_receive);

  if (packet_to_receive.crc_ok == false)
  {
    return false;
  }

  if (!destIsMe())
  {

    if (receiveFrame->duration == 0)
    {
      return false;
    }

    uint16_t wait_time = receiveFrame->duration;
    unsigned long wait_start = micros();

    radio.setIdleState();

    while (micros() - wait_start <= wait_time)
      ;

    radio.setRxState();

    return false;
  }

  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  return true;
}
