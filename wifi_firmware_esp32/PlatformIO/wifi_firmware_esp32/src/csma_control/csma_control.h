#ifndef CSMA_CONTROL_H

#define CSMA_CONTROL_H

#include "../contention_backoff/contention_backoff.h"

class CSMA_CONTROL
{
private:
  /**
   * Chosen contention algorithm
   */
  CONTENTION_BACKOFF *contentionAlgorithm;

  /**
   * number of slots it has to wait until it is free to try
   */
  uint8_t backoffCount;

#ifndef CCA_FROM_GDO2_PIN

  /**
   * number of times this.checkChannel should be checked to cover backoff time
   */
  uint8_t backoffRepetition;

  /**
   * number of times this.checkChannel should be checked to cover sifs time
   */
  uint8_t sifsRepetition;

  /**
   * Checks if channel is free for difs
   *
   * Naturally ocuppies difs time plus whatever extra from not being multiple from checkChannel
   */
  bool difsCheck();

  /**
   * Checks if channel is free for backoff time
   *
   * Naturally ocuppies backoff time plus whatever extra from not being multiple from checkChannel
   */
  bool backoffCheck();

#endif

public:
  /**
   * Constructor
   *
   * 'isChannelFree' function to check if medium is free
   */
  CSMA_CONTROL(CONTENTION_BACKOFF *contentionBackoff);

  ~CSMA_CONTROL();

  /**
   * Waits for its turn to access the channel
   */
  void waitForTurn();

  /**
   * warns class if ack was received or not
   *
   * adjusts contention window accordingly
   */
  void ackReceived(bool wasReceived);
};

#endif
