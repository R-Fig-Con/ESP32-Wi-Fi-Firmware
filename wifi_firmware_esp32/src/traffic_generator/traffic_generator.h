/**
 * Module to generate packet traffic
 */

//Time interval modes
#define TRF_GEN_CONST  (uint8_t)0 //Sets waiting time between packets to be constant
#define TRF_GEN_GAUSS  (uint8_t)1 //Sets waiting time between packets to be decided according a gaussian distribution

#define TRF_GEN_MAX_MSG_LEN 128 //Better to have a small message and just repeat it several times if we want to send a large packet (uses less memory).

struct TRAFFIC_GEN
{
  private:
    //Maybe should contain more destinations, so it can cycle through each of them
    /**
     * Mac address of the receiver
     */
     uint8_t destination_mac[6];// size of a mac

    /**
     * Message to be sent
     */
    uint8_t message[TRF_GEN_MAX_MSG_LEN];

    /**
     * Time interval mode
     */
    uint8_t time_interval_mode = TRF_GEN_CONST;

    /**
     * If mode is TRF_GEN_CONST: the absolute time interval
     * If mode is TRF_GEN_GAUSS: the median time interval
     */
    uint16_t time_interval = 200;

    /**
     * Time to wait before sending again. If mode is constant, it should always equal time_interval
     */
    uint16_t time_to_next = time_interval;

    /**
     * Function to be called to send a packet.
     */
    bool (*sendData)(CCPACKET);

  public:

    /**
     * init
     *
     * Sets up the module and all it's fields. Should set a deault value for message.
     *
     * 'sendDataF' is the function that will be used to send the packet.
     *
     * 'my_addr' is the mac address of the sender.
     *
     * 'destination_addr' will be the address of the receiver.
     */
    void init(bool (* sendDataF)(bool), uint8_t my_addr[6], uint8_t destination_addr[6]);

    /**
    * setTime
    *
    * Set the interval mode and time to custom values.
    *
    * 'time_mode' Will set the time inverval mode of the time between packets.
    * 
    * 'waiting_time' The constant time if mode is TRF_GEN_CONST, or the median time if mode is TRF_GEN_GAUSS
    *
    * returns false if time_mode is invalid; else true
    */
    bool setTime(uint8_t time_mode, uint16_t waiting_time);

    /**
     * Sets message to be sent in each frame.
     */
    void setMessage(uint8_t message[TRF_GEN_MAX_MSG_LEN], uint16_t size);

    /**
     * clearToSend
     *
     * Checks if enough time has passed to be able to send
     * 
     * returns 0 as false, else true
     */
    uint8_t clearToSend();

    /**
     * set_sender
     *
     * Sets the function that will be used to send the packet
     */
    void setSender(bool (* sendDataF)(bool));

};