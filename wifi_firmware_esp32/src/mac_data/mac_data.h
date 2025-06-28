/**
 * statistics fata related to running of mac protocol
 */
struct mac_data
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
     * amount of failures; counts only full fiving up
     */
    volatile uint16_t failures = 0;

};
