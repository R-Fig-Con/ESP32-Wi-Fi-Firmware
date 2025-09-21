#include "../cc1101/cc1101.h"
#include "../csma_control/csma_control.h"
#include "../send_protocol/send_protocol.h"
#include "../traffic_generator/traffic_generator.h"

extern uint8_t myMacAddress[MAC_ADDRESS_SIZE];

extern TRAFFIC_GEN *trf_gen;

extern CSMA_CONTROL *csma_control;

/**
 * no setup needed, exists to be modified when receiving
 */
extern CCPACKET packet_to_receive;
extern ieeeFrame *receiveFrame;

// can be either cts or ack
extern CCPACKET answer_packet;
extern ieeeFrame *answerFrame;

/**
 * probably not a good idea to use either receive/answer variable packets due to misuse could cause
 * packet change from interruption, but could be done
 */
extern CCPACKET rts_packet;
extern ieeeFrame *rtsFrame;

/**
 * calculates amount of time needed at beggining of transmittion
 * Used for rts NAV
 *
 * Assumes as the rest of the code data can be sent in one burst
 *
 * @param data_length length of content from data packet to be sent
 */
uint16_t durationCalculation(unsigned short data_length);

/**
 * semaphore used either to gain access to radio mac protocol
 * wether to read or write
 */
extern SemaphoreHandle_t xSemaphore;

/**
 * Task will handle any communication started by another party
 */
void receiveAndAnswerTask(void *unused_param);

/**
 * Task to change parameters of protocol
 *
 * Not responsible of collecting parameters, receives parameters from queue
 * 
 * TODO consider changing to wifi_config definition
 */
void changeParametersTask(void *unusedParam);

/**
 * Directly affects 'packet_to_receive' global var.
 *
 * Todo add sleep mode during nav instead of NAV. Should requires waiting until it gets idle
 * and use command strobes to change states. Check cc1101 datasheet
 */
bool receiver();