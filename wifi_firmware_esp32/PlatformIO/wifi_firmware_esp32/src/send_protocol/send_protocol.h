#ifndef SEND_PROTOCOL_H
#define SEND_PROTOCOL_H

#include "ccpacket.h"
#include "stdint.h"

/**
 * Base class contains send data function, relevant flags relating to tratement
 * of received packets
 *
 * Note: TODO static bools cannot be initialized here, find alternative
 */
class SEND_PROTOCOL
{

public:
    virtual void send_data(CCPACKET packet_to_send);

};

class CSMA_CA : public SEND_PROTOCOL
{

private:
    /**
     * Boolean deciding if rts/cts is used
     */
    bool rts_cts_used;

    /**
     * number of retries until giving up
     */
    uint8_t retry_limit;

public:
    /**
     * Constructor
     */
    CSMA_CA();

    virtual void send_data(CCPACKET packet_to_send);
};

#endif
