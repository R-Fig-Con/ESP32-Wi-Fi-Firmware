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
 * file to run in place of wifi_firmware.ino, as some use its imports
 *
 * This program will read data written in serial communicaton, which will lead to multiple prints of the state
 *
 * These are to check if the hardware neither comes with flaws or if the radio is well connected
 *
 * Always ensure the serial communication speed is up to date (decided on Serial.begin)
 *
 * Various defines are being kept below as they are expected in other files and removing them
 * would lead to compilation failure
 */

// Removed mac addresses, tests probably will not check them

#define CCA_FROM_GDO2_PIN "Irrelevant value"

/**
 * to uncomment if answer task logic changes with mac protocol parameter change
 */
#define ANSWER_TASK_CHANGES_WITH_PARAMETERS "Irrelevant value"

#define ANSWER_TASK_PRIORITY 10
#define TRAFFIC_GENERATOR_PRIORITY 5
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

/*
 * Array value declaration. Ensure array is big enough for MAC address
 */
// #define DEFAULT_MAC_ADDRESS {0x4C, 0x11, 0xAE, 0x64, 0xD1, 0x8D}

// #define DEFAULT_MAC_ADDRESS {0xBC, 0xDD, 0xC2, 0xCC, 0x3B, 0x31}

#define DEFAULT_MAC_ADDRESS {0x1C, 0x69, 0x20, 0x30, 0xDF, 0x41}

#define DEFAULT_TIME_INTERVAL_MODE TRF_GEN_GAUSS

#define DEFAULT_TIME_INTERVAL 0

/**
 * Enumeration of all existing tests
 */
enum test_choice
{
  /**
   * Simple hello world print from esp to computer.
   *
   *
   * included to have the most simple test for serial communication,
   * something other tests rely on
   */
  ESP_SERIAL_COMMUNICATION = 1,

  /**
   * reading of registers to check if values are as expected
   */
  RADIO_STATES = 2,

  /**
   * Radio communication is acheived check, to be used with COMMUNICATION_RECEIVER
   */
  COMMUNICATION_SENDER = 3,

  /**
   * Radio communication is acheived check, to be used with COMMUNICATION_SENDER
   */
  COMMUNICATION_RECEIVER = 4,
};

CC1101 radio;

byte syncWord[2] = {199, 10};
volatile bool packetWaiting;

void messageReceived()
{
  packetWaiting = true;
}

/**
 * no setup needed, exists to be modified when receiving
 */
CCPACKET packet_to_receive;
ieeeFrame *receiveFrame = (ieeeFrame *)packet_to_receive.data;

// can be either cts or ack
CCPACKET answer_packet;
ieeeFrame *answerFrame = (ieeeFrame *)answer_packet.data;

bool checkChannel()
{
  return radio.cca();
}

/**
 * radio setup used in main code, not including interrupt setup or rx state set
 */
void radio_setup()
{
  radio.init();
  radio.setSyncWord(syncWord);
  radio.setCarrierFreq(CFREQ_433);
  radio.disableAddressCheck();
  radio.setTxPowerAmp(PA_LowPower);

  delay(1000);
}

static char hello_world[] = "Hello World!";
void hello_world_test()
{
  Serial.write(hello_world, strlen(hello_world)); // checked, note does not send \n
}

/**
 * to add more prints, decide if descriptive is kept
 *
 * TODO decide if prints should include text besides measures
 */
void radio_states_test()
{
  radio_setup();

  Serial.print("CC1101_PARTNUM ");
  Serial.println(radio.readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER)); // expects 0
  Serial.print("CC1101_VERSION ");
  Serial.println(radio.readReg(CC1101_VERSION, CC1101_STATUS_REGISTER)); // expects 20
  Serial.print("CC1101_MARCSTATE ");
  Serial.println(radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f); // expects 1

  Serial.print("CC1101_FREQ2 ");
  Serial.println(radio.readReg(CC1101_FREQ2, CC1101_CONFIG_REGISTER)); // expects 16
  Serial.print("CC1101_FREQ1 ");
  Serial.println(radio.readReg(CC1101_FREQ1, CC1101_CONFIG_REGISTER)); // expects 167
  Serial.print("CC1101_FREQ0 ");
  Serial.println(radio.readReg(CC1101_FREQ0, CC1101_CONFIG_REGISTER)); // expects 98

  radio.setRxState();
  delay(10); // to ensure enough time for cmdStrobe to happen

  Serial.print("CC1101_MARCSTATE ");
  Serial.println(radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f); // expects 13
}

CCPACKET packet_to_send;

/**
 * prepares packet_to_send
 */
void sender_create_data_packet()
{
  ieeeFrame *f = (ieeeFrame *)packet_to_send.data;

  memcpy(f->payload, hello_world, strlen(hello_world)); // todo check about \0 size

  packet_to_send.length = strlen((char *)f->payload) + 1 + sizeof(ieeeFrame);

  Serial.print("PACKAGE TO SEND SIZE: ");
  Serial.println(packet_to_send.length);

  PACKET_TO_RTS(f);
}

/**
 * sends packet, waits for ack. Ack is not actually processed
 */
void communication_sender_test()
{
  radio_setup();
  radio.sendData(packet_to_send);
  unsigned long wait_start = micros();

  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  bool give_up = false;

  while (!packetWaiting)
  {
    if (micros() - wait_start >= 4000)
    {
      give_up = true;
      break;
    }
  }

  if (!give_up)
  {
    unsigned long wait_end = micros();
    Serial.printf("Sifs: %lu microsseconds\n", wait_end - wait_start); // expects about 2000? openhtf should include interval
  }

  detachInterrupt(CC1101_GDO0);
}

/**
 * waits for data, sends ack
 *
 * Prints confirm payload of data, crc not ok message
 */
void communication_receiver_test()
{
  radio_setup();
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);
  radio.setRxState();

  while (!packetWaiting)
    ;
  if (receiver())
  {
    radio.sendData(answer_packet);
    Serial.print((char *)receiveFrame->payload);
  }
  else
  {
    Serial.println("Crc not ok");
  }
}

/**
 * Setup  starts serial communication
 */
void setup()
{

  // Serial communication for debug
  Serial.begin(57600);

  delay(1000);

  // answer packet definition
  answer_packet.length = sizeof(ieeeFrame);

  // put it here assuming it will not be changed afterwards
  sender_create_data_packet();
}

char read_buffer[7];
void loop()
{

  while (Serial.available() == 0)
    ;
  int read = Serial.readBytes(read_buffer, 6);
  read_buffer[read] = '\0';

  // Serial.printf("Read value; value in char: %c; value in int: %d\n", (char) read, read);
  // Serial.printf("Buffer read: %s\n", read_buffer);

  switch (atoi(read_buffer))
  {
  case ESP_SERIAL_COMMUNICATION:
    hello_world_test();
    break;
  case RADIO_STATES:
    radio_states_test();
    break;
  case COMMUNICATION_SENDER:
    communication_sender_test();
    break;
  case COMMUNICATION_RECEIVER:
    communication_receiver_test();
    break;
  default:
    Serial.println("Option not recognized");
    break;
  }
}

/**
 * Directly affects 'packet_to_receive' global var
 * returns crc_ok
 */
bool receiver()
{

  // Yes. Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  // Try to receive the packet
  radio.receiveData(&packet_to_receive);
  packetWaiting = false;

  if (packet_to_receive.crc_ok == false)
  {
    return false;
  }

  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  return true;
}
