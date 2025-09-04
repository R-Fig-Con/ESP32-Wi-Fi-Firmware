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

#define CCA_FROM_GDO2_PIN "Irrelevant value"

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

/*
 * Array value declaration. Ensure array is big enough for MAC address
 */
// #define DEFAULT_MAC_ADDRESS {0x4C, 0x11, 0xAE, 0x64, 0xD1, 0x8D}

// #define DEFAULT_MAC_ADDRESS {0xBC, 0xDD, 0xC2, 0xCC, 0x3B, 0x31}

#define DEFAULT_MAC_ADDRESS {0x1C, 0x69, 0x20, 0x30, 0xDF, 0x41}

#define DEFAULT_TIME_INTERVAL_MODE TRF_GEN_GAUSS

#define DEFAULT_TIME_INTERVAL 0

CC1101 radio;

uint8_t myMacAddress[6];

// Packet and frame used by the sender.
CCPACKET packet;
ieeeFrame *frame = (ieeeFrame *)packet.data;

bool packetWaiting;

void messageReceived()
{
  packetWaiting = true;
}

// int id = 1;
int id = 0;

void setup()
{

  // Serial communication for debug
  Serial.begin(57600);

  // Wifi, for getting the MAC address.
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);

  // Initialize the CC1101 radio
  radio.init();
  radio.setCarrierFreq(CFREQ_433);
  radio.disableAddressCheck();
  radio.setTxPowerAmp(PA_LowPower);

  delay(1000);

  Serial.print(F("CC1101_MARCSTATE "));
  Serial.println(radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);

  if (id == 0)
  {
    attachInterrupt(CC1101_GDO0, messageReceived, RISING);
  }

  radio.setRxState();
}

int counter = 0;
void loop()
{

  if (counter == 50)
  {
    Serial.println(F("\n\nReebotting Reebotting Reebotting Reebotting Reebotting Reebotting Reebotting Reebotting"));
    counter = 0;
    /*
    radio.setIdleState();
    while(radio.readStatusReg(CC1101_MARCSTATE) != RFSTATE_IDLE);
    Serial.println(F("SET ON IDLE"));
    radio.flushRxFifo();
    radio.setRxState();
    */

    /*
    detachInterrupt(CC1101_GDO0);
    radio.reset();
    attachInterrupt(CC1101_GDO0, messageReceived, RISING);
    radio.setRxState();
    */

    Serial.println(F("Reeboted Reeboted Reeboted Reeboted Reeboted Reeboted Reeboted Reeboted\n\n"));
  }
  if (id)
  {
    delay(10000);
    Serial.println(F("JAMMING"));
    radio.jamming();
  }
  else
  {
    counter += 1;
    bool free = radio.cca();
    int8_t rssi = (int8_t)radio.readStatusReg(CC1101_RSSI);

    if (free)
    {
      Serial.println(F("\n\nRead cca; is free"));
    }
    else
    {
      Serial.println(F("\n\nRead cca; is NOT FREE"));
    }

    Serial.print(F("Read rssi; value:"));
    Serial.print(rssi);

    Serial.print(F(";\tRead state; value:"));
    Serial.print(radio.readStatusReg(CC1101_MARCSTATE));

    Serial.print(F("\tRead gdo2 state; value:"));
    Serial.println(radio.readConfigReg(CC1101_IOCFG2));

    delay(200);
    receiver();
  }

  // Serial.println("Loop end");
}

void receiver()
{

  // Is there a packet to be received?
  if (!packetWaiting)
    return;

  // Serial.println("RECEIVEE");

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  if (radio.receiveData(&packet) > 0)
    Serial.println(F("Received packet..."));

  // We received something, but is the packet correct?
  if (!packet.crc_ok)
  {
    Serial.println(F("crc not ok"));
  }

  // Print some physical layer statistics
  Serial.print(F("lqi: "));
  Serial.println(radio.raw2lqi(packet.lqi));
  Serial.print(F("rssi: "));
  Serial.print(radio.raw2rssi(packet.rssi));
  Serial.println(F("dBm"));

  // Mark the packet as processed and re-enable the interruption.
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);
}