/**
 * list of backoff protocols from which to choose
*/
enum BACKOFF_PROTOCOLS{
  MILD,
  LINEAR
};

/**
 * parameter collection for trafficGenerator
*/
struct trafficGeneratorParameters{
  /**
   * flag to communicate if values ware actually given
  */
  bool used;

  uint8_t time_mode;

  uint16_t waiting_time;
};


struct csmaControlParameters{
  /**
   * flag to communicate if values ware actually given
  */
  bool used;


  /*
   * wether backof is of MILD, Exponentional, or other.
   *
  */
  BACKOFF_PROTOCOLS backoff_protocol;
};



/**
 * Contains all possible data used to reconfigure mac protocol
*/
struct macProtocolParameters{
  trafficGeneratorParameters traf_gen_params;

  csmaControlParameters csma_contrl_params;
};

/**
 * Handle for a queue containing type macProtocolParameters
 */
QueueHandle_t protocolParametersQueueHandle = xQueueCreate( 1, sizeof(macProtocolParameters) );