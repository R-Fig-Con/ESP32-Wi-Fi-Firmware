#define BACKOFF_TIME_SLOT 20 //random value again
#define MAX_BACKOFF_RANGE 2048 //maybe should be a variable for performance testing purposes

//For nav duration these should probably be declared somewhere else
#define SIFS 20 //random value
#define DIFS SIFS + (2*BACKOFF_TIME_SLOT)   //from wikipedia definition
#define EIFS SIFS + DIFS //wiki definition: Transmission time of Ack frame at lowest phy mandatory rate + SIFS + DIFS



class CSMA_CONTROL
{
   private:
      /**
        * maximum backoff slots it currently allows
        */
       uint8_t contentionWindow = 2;

       /**
        * number of slots it has to wait until it is free to try
        */
       uint8_t backoffCount;

       /**
        * function to check if channel is free
        * 
        * returns true if is free, false otherwise
        */
       bool (*checkChannel)();//TODO discuss responsibility for GDO2 register configuration

   public:

       /**
        * Constructor
        *
        * 'isChannelFree' function to check if medium is free
        */
       CSMA_CONTROL(bool (*isChannelFree)());

       /**
        * Waits for its turn to access the channel
        */
       void waitForTurn();

       /**
        * warns class id ack was received or not
        * 
        * adjusts contention window accordingly
        */
       void ackReceived(bool wasReceived);

};