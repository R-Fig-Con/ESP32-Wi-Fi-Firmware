/**
 * Module to generate packet traffic
 */

//Global constant
#define MAC_ADDRESS_SIZE (uint8_t)6

//Time interval modes
#define TRF_GEN_CONST  (uint8_t)0 //Sets waiting time between packets to be constant
#define TRF_GEN_GAUSS  (uint8_t)1 //Sets waiting time between packets to be decided according a gaussian distribution

#define TRF_GEN_MAX_MSG_LEN 128 //Better to have a small message and just repeat it several times if we want to send a large packet (uses less memory).

//identifying values in frame control. Taken from book, page 100
//encapsulates type and subtype
#define ACK_TYPE_VALUE 0x1D
#define RTS_TYPE_VALUE 0x1C
#define CTS_TYPE_VALUE 0x1B
#define DATA_TYPE_VALUE 0x20 //does not use subtype?

//Overriding protocol version
#define PACKET_TO_ACK(f) (f->frame_control[0] = ACK_TYPE_VALUE)
#define PACKET_TO_CTS(f) (f->frame_control[0] = CTS_TYPE_VALUE)
#define PACKET_TO_RTS(f) (f->frame_control[0] = RTS_TYPE_VALUE)
#define PACKET_TO_DATA(f) (f->frame_control[0] = DATA_TYPE_VALUE)   

#define PACKET_IS_ACK(f) (f->frame_control[0] == ACK_TYPE_VALUE)
#define PACKET_IS_RTS(f) (f->frame_control[0] == RTS_TYPE_VALUE)
#define PACKET_IS_CTS(f) (f->frame_control[0] == CTS_TYPE_VALUE)
#define PACKET_IS_DATA(f) (f->frame_control[0] == DATA_TYPE_VALUE)

/**
 * Frame format.
 */
struct ieeeFrame {

    uint8_t   frame_control[2] = {0};

    // in microseconds
    uint16_t   duration;

    // Destination address
    uint8_t   addr_dest[MAC_ADDRESS_SIZE];

    // Source address
    uint8_t   addr_src[MAC_ADDRESS_SIZE];

    // BSSID
    uint8_t   bssid[6];

    // Sequence control
    uint8_t   seq_ctr[6];

    // Address 4 - N/A
    const uint8_t   addr_extr[MAC_ADDRESS_SIZE] = {0};

    // Network data
    uint8_t   payload[0];
};

class TRAFFIC_GEN
{
  private:
    CCPACKET packet;

    /**
     * Time interval mode
     */
    uint8_t time_interval_mode = TRF_GEN_CONST;

    /**
     * If mode is TRF_GEN_CONST: the absolute time interval
     * If mode is TRF_GEN_GAUSS: the median time interval
     * Is in millisseconds
     */
    uint16_t time_interval = 1000;

    /**
     * Time to wait before sending again.
     */
    uint16_t time_to_next;

    /**
     * Indicates if the module is running.
     */
    bool running;

    /**
     * Function to be called to send a packet.
     */
    void (*sendData)(CCPACKET);

    /**
    * setTime
    *
    * Gets a random value based on normal distribution using the time_interval as the mean
    *
    * returns value
    */
    uint16_t getTimeFromGauss();

  public:

    /**
     * Constructor
     *
     * Sets up the module and all it's fields. Should set a deault value for message.
     *
     * @param 'sendDataF' is the function that will be used to send the packet.
     *
     * @param 'my_addr' is the mac address of the sender.
     *
     * @param 'destination_addr' will be the address of the receiver.
     * 
     * @param 'duration' calculated value for ieeeFrame.duration
     * 
     * @param 'data_length' length of data. Should contain size for ieeeFrame fields
     */
    TRAFFIC_GEN(void (* sendDataF)(CCPACKET), uint8_t my_addr[MAC_ADDRESS_SIZE], uint8_t destination_addr[MAC_ADDRESS_SIZE], uint16_t duration, uint16_t data_length);

    /**
     * Constructor
     *
     * Specifies the specific message desired
     *
     * @param 'sendDataF' is the function that will be used to send the packet.
     *
     * @param 'my_addr' is the mac address of the sender.
     *
     * @param 'destination_addr' will be the address of the receiver.
     * 
     * @param 'duration' calculated value for ieeeFrame.duration
     * 
     * @param 'message_length' length of message
     * 
     * @param 'message' pointer to message
     */
    TRAFFIC_GEN(void (* sendDataF)(CCPACKET), uint8_t my_addr[MAC_ADDRESS_SIZE], uint8_t destination_addr[MAC_ADDRESS_SIZE], uint16_t duration, uint16_t data_length, char* message);

    /**
     * init
     *
     * Starts generating packets. Once this is called, the fields will not be changeble.
     *
     * Return false if stopped unexpectadly; else true.
     *
     */
    bool init();

    /**
     * stop
     *
     * Stops generating packets. Once this is called, the fields should be changeble.
     *
     */
    void stop();

    /**
     * isRunning
     *
     * Returns true if it is generating packets..
     *
     */
    bool isRunning();

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
     * setter for destination address
     * 
     * @param 'addr' new destination address
     */
    void setDestAddress(uint8_t addr[MAC_ADDRESS_SIZE]);

    /**
     * sets the new message contents. Specifies message content for testing purposes
     * 
     * @param 'message' new message
     * 
     * @param 'message_length' new message's length
     *
     */
    void setMessage(char* message, uint16_t message_length);

};