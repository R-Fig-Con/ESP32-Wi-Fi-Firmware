#include "wifi_firmware_esp32.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "WiFi.h"

#include "radio_interruption.h"
#include "mac_data.h"
#include "interframe_spaces.h"
#include "./wifi_config/wifi_config.h"

byte syncWord[2] = {199, 10};

TaskHandle_t receiveHandle = NULL;

volatile bool packetWaiting = false;
bool automaticResponse = true;

void messageReceived()
{
  if (automaticResponse)
  {
    portYIELD_FROM_ISR(xTaskResumeFromISR(receiveHandle));
  }
  else
  {
    packetWaiting = true;
  }
}

/**
 * calculation of duration filed for NAV found on the data
 */
uint16_t dataDurationCalculation()
{
  return CC1101::radio->transmittionTime(answer_packet) + SIFS; // 1 SIFS + ack time
}

void setup()
{

  // Serial communication for debug
  // should match value on .ini file
  Serial.begin(57600);

  // Wifi, for getting the MAC address.
  WiFi.mode(WIFI_STA);
  // WiFi.STA.begin(); -- No need to start WiFi to get MAC
  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);

  delay(1000);

  PRINT("My MAC:");
  PRINTLN_MAC(myMacAddress);

  // Initialize the CC1101 radio
  CC1101::radio->init();
  CC1101::radio->setSyncWord(syncWord);
  CC1101::radio->setCarrierFreq(CFREQ_433);
  CC1101::radio->disableAddressCheck();
  CC1101::radio->setTxPowerAmp(PA_LowPower);

  delay(1000);

  // Print some debug info
  PRINT("CC1101_PARTNUM ");
  PRINTLN_VALUE(CC1101::radio->readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER));
  PRINT("CC1101_VERSION ");
  PRINTLN_VALUE(CC1101::radio->readReg(CC1101_VERSION, CC1101_STATUS_REGISTER));
  PRINT("CC1101_MARCSTATE ");
  PRINTLN_VALUE(CC1101::radio->readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);

  PRINTLN_VALUE("CC1101 radio initialized.");

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

  csma_control = new CSMA_CONTROL(CONTENTION_BACKOFF::getBackoffProtocol(DEFAULT_BACKOFF_ALGORITHM));

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

  trf_gen = new TRAFFIC_GEN(new CSMA_CA(), myMacAddress, dstMacAddress, dataDurationCalculation(), sizeof(ieeeFrame) + DEFAULT_FRAME_CONTENT_SIZE);
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

  attach_radio_interrupt();

  CC1101::radio->setRxState();
}

TaskHandle_t generatorHandle = NULL;

void generatorTask(void *unusedParam)
{
  trf_gen->init(); // is already a loop. Maybe change to add init restart on isRunning == false?
}

void loop()
{
  //xthal_dcache_line_invalidate();

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
