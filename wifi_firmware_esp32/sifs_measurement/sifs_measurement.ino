#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/cc1101_driver/cc1101_driver.h"
#include "src/cc1101_driver/ccpacket.h"
#include "src/traffic_generator/traffic_generator.h"
#include "src/csma_control/csma_control.h"

/**
 * This test requires setting manually the mode of the two nodes on the nodeOpModee global variable
 */

enum nodeOpMode
{
  NODE_OP_MODE_RECEIVER,   // answers
  NODE_OP_MODE_TRANSMITTER // starts and measures
};

CC1101 radio;

byte syncWord[2] = {199, 10};
volatile bool packetWaiting;

nodeOpMode opMode;
uint8_t myMacAddress[6];
CCPACKET packet_to_send;

TaskHandle_t receiveHandle = NULL;

/**
 * no setup needed, exists to be modified when receiving
 */
CCPACKET packet_to_receive;
ieeeFrame *receiveFrame = (ieeeFrame *)packet_to_receive.data;

// can be either cts or ack
CCPACKET answer_packet;
ieeeFrame *answerFrame = (ieeeFrame *)answer_packet.data;

unsigned long time_marker = 0;

void messageReceived()
{
  time_marker = micros();
  packetWaiting = true;
}

void answerInterrupt()
{
  portYIELD_FROM_ISR(xTaskResumeFromISR(receiveHandle));
}

void receiveAndAnswerTask(void *unused_param)
{

  while (true)
  {

    vTaskSuspend(receiveHandle); // suspend self; done on activation and after each receive

    Serial.println(F("ANSWER TASK awakened"));
    if (!receiver())
    {
      Serial.println(F("answer task: bad package"));
      continue;
    }

    if (PACKET_IS_DATA(receiveFrame))
    {
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_ACK(answerFrame);
      // Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      // Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if (!radio.sendData(answer_packet))
      {
        Serial.println("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, answerInterrupt, RISING);

      Serial.println("SENT ACK");
    }
    else if (PACKET_IS_RTS(receiveFrame))
    {
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_CTS(answerFrame);
      // Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      // Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if (!radio.sendData(answer_packet))
      {
        Serial.println("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, answerInterrupt, RISING);

      Serial.println("SENT CTS");
    }
    else
    {
      Serial.printf("Response task, frame control %d; or 0x%x (hex) was not recognized\n", (uint)receiveFrame->frame_control[0], receiveFrame->frame_control[0]);
    }
  }

  Serial.println(F("Out of while loop. Should be impossible"));
}

void setup()
{
  // Serial communication for debug
  Serial.begin(115200);

  delay(1000);

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
  // opMode = NODE_OP_MODE_TRANSMITTER;

  // Serial.print(F("Size of ieeeFrame struct: ")); Serial.println(sizeof(ieeeFrame));

  if (opMode == NODE_OP_MODE_RECEIVER)
  {
    xTaskCreatePinnedToCore(
        &receiveAndAnswerTask,
        "receive and send acknowledge",
        10000, // no thought used to decide size
        NULL,
        10, // should be bigger than traffic generation
        &receiveHandle,
        1 // putting related to cc1101 on same core
    );

    Serial.println(F("ANSWER TASK CREATED"));

    attachInterrupt(CC1101_GDO0, answerInterrupt, RISING); // activates interrupt, never disables it
  }
  else
  {
    sender_create_data_packet(&packet_to_send);
    Serial.println(F("Starts communication, waits for answer"));
  }

  radio.setRxState();
}

void loop()
{

  if (opMode == NODE_OP_MODE_TRANSMITTER)
  {
    sendAndReceive();
    delay(40000);
  }
}

void sender_create_data_packet(CCPACKET *packet)
{
  unsigned int seqNum = 0;
  ieeeFrame *f = (ieeeFrame *)packet->data;

  // Fill the MAC address.
  memcpy(f->addr_src, myMacAddress, 6);

  // Fill the payload. Start by adding a sequence number.
  // TODO: We will certainly want this sequence number to be
  // binary in the final version. We are using strings
  // here just to simplify debug.
  //    sprintf((char *) f->payload, "%08X ", seqNum++);

  // Now, fill the remainder of the payload with some
  // data (just to have something that is verificable on
  // the receiver end).
  // TODO: this can be safely removed on the final version,
  // as the payload itself is not going to be important.
  int l = 0;
  for (int i = 0; i < 2000; i++)
  {
    f->payload[l++] = (char)((i % 42) + 48);
  }
  f->payload[l] = (char)0;

  // Set the packet length. It is the length of the payload
  // because it is a string, remember to count the \0 at the end,
  // plus the 6 bytes of the MAC address.
  packet->length = strlen((char *)f->payload) + 1 + 6;

  Serial.print("PACKAGE TO SEND SIZE: ");
  Serial.println(packet->length);

  PACKET_TO_RTS(f);
}

void sendAndReceive()
{
  Serial.printf("Send and receive, marker: %lu\n", time_marker);
  radio.sendData(packet_to_send);
  unsigned long wait_start = micros();
  // Serial.println(F("TODO DELETE SOLVE TEST"));
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  unsigned long c = 0;
  while (!packetWaiting)
  {
    // Serial.println(F("LOOP\n"));// not necessary if is volatile
    c += 1;
  }

  Serial.printf("Initial sender; Took %lu microsseconds from packet send to answer, loop count %lu;\n", time_marker - wait_start, c);

  time_marker = 0;
  detachInterrupt(CC1101_GDO0);
}

/**
 * Directly affects 'packet_to_receive' global var
 * returns CCPACKET.crc_ok
 *
 * Even if attavhes messageReceived for answer too, it makes no difference in this case since it will be removed after
 */
bool receiver()
{

  // Disable the reception interruption while we handle this packet.
  detachInterrupt(CC1101_GDO0);

  radio.receiveData(&packet_to_receive);

  // Serial.println(packet_to_receive.length);

  // only necessary if is not handled by task; probably should be placed somewhere else; TODO
  packetWaiting = false;
  attachInterrupt(CC1101_GDO0, messageReceived, RISING);

  return packet_to_receive.crc_ok;
}