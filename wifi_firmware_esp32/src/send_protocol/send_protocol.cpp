#include "send_protocol.h"
#include "../utils/utils.h"

#include "interframe_spaces.h"
#include "ccpacket.h"
#include "mac_data.h"
#include "radio_interruption.h"

CSMA_CA::CSMA_CA()
{
  this->rts_cts_used = true;

  this->retry_limit = 10;
}

void CSMA_CA::send_data(CCPACKET packet_to_send)
{
  PRINTLN("TO SEND");
  uint8_t retryCount = 0;
  unsigned long start_time; // used for both rts and data wait
  xSemaphoreTake(xSemaphore, portMAX_DELAY);

// Label
send:
  // should be able to answer while waiting for turn, so it cannot be deactivated
  if (retryCount == this->retry_limit)
  {
    PRINTLN("GIVING UP after retry limit reached");
    xSemaphoreGive(xSemaphore);
    mac_data.failures += 1;
    return;
  }

  csma_control->waitForTurn();

  if (!this->rts_cts_used)
  {
    goto data_send;
  }

  remove_radio_interrupt();
  CC1101::radio->sendData(rts_packet);
  attach_radio_interrupt();

  automaticResponse = false;
  start_time = micros();
  while (!packetWaiting)
  {
    // sifs wait
    if (micros() - start_time >= SIFS)
    {
      PRINTLN("WAIT FOR CTS FAILED");
      retryCount += 1;
      mac_data.retries += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }
  }
  packetWaiting = false;
  // checks if is ok and is an ack ack
  if (receiver() && !PACKET_IS_CTS(receiveFrame))
  {
    PRINTLN("answer NOT a CTS");
    retryCount += 1;
    mac_data.retries += 1;
    csma_control->ackReceived(false);
    goto send;
  }

data_send:

  remove_radio_interrupt();
  CC1101::radio->sendData(packet_to_send);
  attach_radio_interrupt();
  automaticResponse = false; // necessary to do here if rts/cts skipped
  start_time = micros();
  while (!packetWaiting)
  {
    // sifs wait
    if (micros() - start_time >= SIFS)
    {
      PRINTLN("No ack");
      retryCount += 1;
      mac_data.retries += 1;
      automaticResponse = true;
      csma_control->ackReceived(false);
      goto send;
    }
  }
  packetWaiting = false;
  automaticResponse = true; // not necessary here? todo check

  // checks if is ok and is an ack ack
  if (receiver() && !PACKET_IS_ACK(receiveFrame))
  {
    PRINTLN("answer is NOT an ACK");
    retryCount += 1;
    mac_data.retries += 1;
    csma_control->ackReceived(false);
    goto send;
  }

  csma_control->ackReceived(true);
  xSemaphoreGive(xSemaphore);
  mac_data.successes += 1;
  PRINTLN("Complete success");
}
