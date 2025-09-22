#include "arduinoGlue.h"
#include "utils.h"
#include "../wifi_config/param_data.h"
#include "mac_data.h"
#include "radio_interruption.h"
#include "interframe_spaces.h"
#include "prints.h"

#include "Arduino.h"

uint8_t myMacAddress[MAC_ADDRESS_SIZE];

TRAFFIC_GEN *trf_gen;

CSMA_CONTROL *csma_control;

CCPACKET packet_to_receive;
ieeeFrame *receiveFrame = (ieeeFrame *)packet_to_receive.data;

CCPACKET answer_packet;
ieeeFrame *answerFrame = (ieeeFrame *)answer_packet.data;

CCPACKET rts_packet;
ieeeFrame *rtsFrame = (ieeeFrame *)rts_packet.data;

SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();

uint16_t durationCalculation(unsigned short data_length)
{
  CCPACKET data_packet;
  data_packet.length = sizeof(ieeeFrame) + data_length;
  return CC1101::radio->transmittionTime(data_packet) + (2 * CC1101::radio->transmittionTime(answer_packet)) + 3 * SIFS; // 3 SIFS, ack and cts
}

/**
 * Checks if destintation of message is for the node. Assumes read packet is packet_to_receive
 *
 * Return true if dest is for the node, false oterwise
 */
inline bool destIsMe()
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

bool receiver()
{

  // Yes. Disable the reception interruption while we handle this packet.
  remove_radio_interrupt();

  // Try to receive the packet
  CC1101::radio->receiveData(&packet_to_receive);

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

    CC1101::radio->setIdleState();

    while (micros() - wait_start <= wait_time)
      ;

    CC1101::radio->setRxState();

    return false;
  }

  attach_radio_interrupt();

  return true;
}


void receiveAndAnswerTask(void *unused_param)
{

  while (true)
  {

    vTaskSuspend(NULL); // suspend self; done on activation and after each receive

    if (!receiver())
    {
      continue;
    }

    PRINT("DST: ");
    PRINTLN_MAC(receiveFrame->addr_dest);

    // Set destination address
    memcpy(answerFrame->addr_dest, receiveFrame->addr_src, MAC_ADDRESS_SIZE);

    // PRINTLN("Received frame on response task");

    if (PACKET_IS_DATA(receiveFrame))
    {
      remove_radio_interrupt();

      PACKET_TO_ACK(answerFrame);
      answerFrame->duration = 0; // no more to send after this, since fragmentation is not supported
      // Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      // Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if (!CC1101::radio->sendData(answer_packet))
      {
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attach_radio_interrupt();

      PRINTLN("SENT ACK");
    }
    else if (PACKET_IS_RTS(receiveFrame))
    {
      remove_radio_interrupt();

      PACKET_TO_CTS(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - CC1101::radio->transmittionTime(packet_to_receive);
      // Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      // Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if (!CC1101::radio->sendData(answer_packet))
      {
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attach_radio_interrupt();

      PRINTLN("SENT CTS");
    }
    else
    {
      PRINT("Response task, frame control not recognized, with value: ");
      PRINTLN_VALUE(receiveFrame->frame_control[0]);
    }
  }
}

void changeParametersTask(void *unusedParam)
{

  // Implementing queue solution for now

  PRINTLN("Created change parameters task");

  uint8_t params_buffer[sizeof(macProtocolParameters)];

  macProtocolParameters *params = (macProtocolParameters *)params_buffer;

  while (true)
  {

    /**
     * check if portMAX_DELAY wait is forever
     */
    if (xQueueReceive(protocolParametersQueueHandle, params_buffer, portMAX_DELAY) == pdFALSE)
    {
      continue;
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    CC1101::radio->setIdleState(); // to avoid rx overflow
    PRINTLN("Set to idle state, avoiding answer task being activated");
#endif

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    PRINTLN("\n\nCHANGE PARAMETERS GOT EXCLUSIVITY\n\n");

    mac_data.retries = 0;
    mac_data.failures = 0;
    mac_data.successes = 0;
    mac_data.startTime = millis();

    if (params->csma_contrl_params.used)
    {
      delete csma_control;
      csma_control = new CSMA_CONTROL(CONTENTION_BACKOFF::getBackoffProtocol(params->csma_contrl_params.backoff_protocol));
    }

    if (params->traf_gen_time.used)
    {
      trf_gen->setTime(params->traf_gen_time.time_mode, params->traf_gen_time.waiting_time);
    }

    if (params->traf_gen_addr.used)
    {
      PRINT("CHANGE PARAMS TASK NEW DEST MAC: ");
      PRINTLN_MAC(params->traf_gen_addr.address);
      trf_gen->setDestAddress(params->traf_gen_addr.address);
      memcpy(rtsFrame->addr_dest, params->traf_gen_addr.address, MAC_ADDRESS_SIZE);
    }

    if (params->traf_gen_data.used)
    {
      Serial.println("Changing message");

      rtsFrame->duration = durationCalculation(params->traf_gen_data.message_length);
      trf_gen->setMessage(
          params->traf_gen_data.message,
          params->traf_gen_data.message_length);

      // TODO find better solution than malloc and free if possible
      free(params->traf_gen_data.message);
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    CC1101::radio->setRxState();
    PRINTLN("On rx state, allowing answer task activating");
#endif

    xSemaphoreGive(xSemaphore);
    PRINTLN("CHANGED, delay\n");
  }
}