#define SIFS 2100 //Per most current measurement, should be at least 2000

#define BACKOFF_TIME_SLOT 1182 //following proprtion of ieee 802.11a between sifs and backoff slot
#define MAX_BACKOFF_RANGE 2048 //maybe should be a variable for performance testing purposes

//For nav duration these should probably be declared somewhere else

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
        * 
        * Warning: function no longer lasts for backoff slot time
        */
       bool (*checkChannel)();

       /**
        * number of times this.checkChannel should be checked to cover backoff time
        */
       uint8_t backoffRepetition;

       /**
        * number of times this.checkChannel should be checked to cover sifs time
        */
       uint8_t sifsRepetition;


       /**
        * Checks if channel is free for difs
        * 
        * Naturally ocuppies difs time plus whatever extra from not being multiple from checkChannel
        */
       bool difsCheck(){
  
        for(uint8_t k = 0; k < this->sifsRepetition + this->backoffRepetition; k += 1){
          if(!this->checkChannel())
            return false;
        }
      
        return true;
       }

       /**
        * Checks if channel is free for backoff time
        * 
        * Naturally ocuppies backoff time plus whatever extra from not being multiple from checkChannel
        */
       bool backoffCheck(){
        for(uint8_t k = 0; k < this->backoffRepetition; k += 1){
          if(!this->checkChannel())
            return false;
        }
      
        return true;
       }

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
        * warns class if ack was received or not
        * 
        * adjusts contention window accordingly
        */
       void ackReceived(bool wasReceived);

};