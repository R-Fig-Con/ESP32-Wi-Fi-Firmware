
class CONTENTION_BACKOFF{
    protected:
         /**
          * minimum value contention window allows
          */
         uint8_t minimum;

         /**
          * current contention window
          */
         uint8_t contentionWindow;

         /**
          * max value contention window allows
          */
         uint16_t maximum;
         
    public:

         /**
          * returns random backoff value
          */
         uint8_t getBackoff(){
            //https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
            //minimum as 0
            return rand() / (RAND_MAX / (contentionWindow + 1) + 1);
         }

         /**
          * reduce contention window as desired by implementer
          */
         virtual void reduceContentionWindow();

         /**
          * increase contention window as desired by implementer
          */
         virtual void increaseContentionWindow();
};