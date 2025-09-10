#include "radio_pins.h"

#define attach_radio_interrupt() attachInterrupt(CC1101_GDO0, messageReceived, RISING);
#define remove_radio_interrupt() detachInterrupt(CC1101_GDO0);

//declared here, implemented on main
/**
 * volatile might have only been necessary to force re-read on empty while loop
 * leaving only on value change. Might be safe to remove
 */
extern volatile bool packetWaiting;

/**
 * indicates to the code if packet should use automatic response
 * or if is expecting data and code will linearly deal with it
 */
extern bool automaticResponse;

void messageReceived();