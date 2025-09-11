#ifndef ARDUINOGLUE_H
#define ARDUINOGLUE_H

//============ Includes ====================
#include <Arduino.h> // for prints, interrupts, could be used for millis and micros, among others

//============ Defines & Macros====================

#define CCA_FROM_GDO2_PIN
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

/**
 * When defined, code is on debug mode, and prints values with the defines
 *
 * If not defined code should not do prints
 */
#define MONITOR_DEBUG_MODE // Does not need a value

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
