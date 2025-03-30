#include "src/csma_control/csma_control.h"  //is necessary this time?

//https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
int random(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

CSMA_CONTROL::CSMA_CONTROL(bool (*isChannelFree)()){
    this->checkChannel = isChannelFree;

    this->backoffCount = random(0, this->contentionWindow);
    
}


void CSMA_CONTROL::waitForTurn(){
    this->checkChannel();

    //

    delayMicroseconds(DIFS);
}


void CSMA_CONTROL::ackReceived(bool wasReceived){
    if (wasReceived)
    {
        if (this->contentionWindow == MAX_BACKOFF_RANGE)//maximum
        this->contentionWindow >>= 1;
    }
    else{
        if (this->contentionWindow == 2)//assumed minimum
            this->contentionWindow <<= 1;
    }
    
    this->backoffCount = random(0, this->contentionWindow);

}
