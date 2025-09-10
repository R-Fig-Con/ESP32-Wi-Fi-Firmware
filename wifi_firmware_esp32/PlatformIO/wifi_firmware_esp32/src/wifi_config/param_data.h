#include "stdint.h"
#include "mac_address_size.h"

#include "../contention_backoff/contention_backoff.h"
//#include <queue.h>

/**
 * parameter collection for trafficGenerator
 */
struct trafficGeneratorTimeParameters
{
  /**
   * flag to communicate if values ware actually given
   */
  bool used = false;

  uint8_t time_mode;

  uint16_t waiting_time;
};

struct csmaControlParameters
{
  /**
   * flag to communicate if values ware actually given
   */
  bool used = false;

  /*
   * wether backof is of MILD, Exponentional, or other.
   *
   */
  BACKOFF_PROTOCOLS backoff_protocol;
};

struct trafficGeneratorAddressParameter
{
  /**
   * flag to communicate if values ware actually given
   */
  bool used = false;

  /**
   * new address
   */
  uint8_t address[MAC_ADDRESS_SIZE];
};

struct trafficGeneratorDataParameter
{
  /**
   * flag to communicate if values ware actually given
   */
  bool used = false;

  /**
   * new message
   *
   * Warning: passing message as a pointer without specific length will require malloc and free
   */
  char *message = 0;

  /**
   * length of message
   */
  uint16_t message_length;
};

/**
 * Contains all possible data used to reconfigure mac protocol
 */
struct macProtocolParameters
{
  trafficGeneratorTimeParameters traf_gen_time;

  trafficGeneratorAddressParameter traf_gen_addr;

  trafficGeneratorDataParameter traf_gen_data;

  csmaControlParameters csma_contrl_params;
};

/**
 * Handle for a queue containing type macProtocolParameters
 */
extern QueueHandle_t protocolParametersQueueHandle;