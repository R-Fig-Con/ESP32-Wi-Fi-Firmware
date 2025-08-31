/**
 * Base class contains send data function, relevant flags relating to tratement
 * of received packets
 * 
 * Note: TODO static bools cannot be initialized here, find alternative
 */
class SEND_PROTOCOL{

    protected:
        volatile bool packetWaiting = false;

        /**
         * indicates to the code if packet should use automatic response
         * or if is expecting data and code will linearly deal with it
        */
        bool automaticResponse = true;

    public:
        virtual void send_data(CCPACKET packet_to_send);

        bool give_automatic_response(){
            return automaticResponse;
        }

        bool set_packet_flag(){
            return packetWaiting = true;
        }

};

class CSMA_CA: public SEND_PROTOCOL{

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