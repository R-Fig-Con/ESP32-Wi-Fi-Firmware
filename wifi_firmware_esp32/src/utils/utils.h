CC1101 radio;

uint8_t myMacAddress[MAC_ADDRESS_SIZE];

TRAFFIC_GEN * trf_gen;

CSMA_CONTROL * csma_control;

SEND_PROTOCOL * send_protocol;

/**
 * no setup needed, exists to be modified when receiving
*/
CCPACKET packet_to_receive;
ieeeFrame * receiveFrame = (ieeeFrame *) packet_to_receive.data;

//can be either cts or ack
CCPACKET answer_packet; 
ieeeFrame * answerFrame = (ieeeFrame *) answer_packet.data;

/**
 * probably not a good idea to use either receive/answer variable packets due to misuse could cause
 * packet change from interruption, but could be done
*/
CCPACKET rts_packet;
ieeeFrame * rtsFrame = (ieeeFrame *) rts_packet.data;

/**
 * semaphore used either to gain access to radio mac protocol
 * wether to read or write
*/
SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();

/**
 * Task will handle any communication started by another party
 */
void receiveAndAnswerTask(void* unused_param);

/**
 * Task to change parameters of protocol
 * 
 * Not responsible of collecting parameters, receives parameters from queue
*/
void changeParametersTask(void* unusedParam);