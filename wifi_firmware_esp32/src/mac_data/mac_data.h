/**
 * statistics fata related to running of mac protocol
 */
struct
{
    /**
     * log of start time for the mac protocol. 
     * Used to calculate for how low protocol is running on device
     * Interpreted as microsseconds
     */
    volatile unsigned long startTime = 0;

    /**
     * amount of successes;
     */
    volatile uint16_t successes = 0;

    /**
     * amount of failures; counts only fully fiving up
     */
    volatile uint16_t failures = 0;

    /**
     * number of times csma/ca needs to restart, either due to not receiving either cts or ack
     */
    volatile uint32_t retries = 0;

} mac_data;
