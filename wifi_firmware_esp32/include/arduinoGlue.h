#ifndef ARDUINOGLUE_H
#define ARDUINOGLUE_H

//============ Includes ====================
#include <Arduino.h> // for prints, interrupts, could be used for millis and micros, among others
#include "cc1101_option.h" // define choice

//============ Defines & Macros====================

/**
 * code contains task to answer incoming packets.
 *
 * If parameter change modifies logic of answer (attempting to moify the radio
 * may also be a condition?) this macro should exist and code should safeguard
 * while doing changes
 */
#define ANSWER_TASK_CHANGES_WITH_PARAMETERS

#define ANSWER_TASK_PRIORITY 10
#define TRAFFIC_GENERATOR_PRIORITY 5
/**
 * Note (27 august) with current traffic generator, if delay value is 0 parameter
 * change task will not run when its priority is lower than the generator's
 */
#define PARAMETER_CHANGE_PRIORITY 6

/**
 * If not 0 watchdog from core 0 is activated, since it contains blocking function
 */
#define WIFI_TASK_PRIORITY 0
#define LOOP_PRIORITY 1

// default values
#define DEFAULT_BACKOFF_ALGORITHM CONSTANT
/**
 * content does not include size for frame control bits (ieeeFrame)
 */
#define DEFAULT_FRAME_CONTENT_SIZE 1000
#define DEFAULT_MAC_ADDRESS {0x1C, 0x69, 0x20, 0x30, 0xDF, 0x41}
#define DEFAULT_TIME_INTERVAL_MODE TRF_GEN_GAUSS
#define DEFAULT_TIME_INTERVAL 5000

#endif // ARDUINOGLUE_H
