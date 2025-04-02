#include "src/csma_control/csma_control.h"  //is necessary this time?

//https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
int random(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

CSMA_CONTROL::CSMA_CONTROL(bool (*isChannelFree)()){
    this->checkChannel = isChannelFree;

    this->backoffCount = random(0, this->contentionWindow);
    
}


/**      ____________________
 *      /                    \
 *     /                      \
 *    |     channel used       | <----
 *     \                      /      |
 *      \____________________/       |
 *                 |                 |
 *                 | check slot      |
 *                 |                 |
 *              ________             |
 *              |       |       No   |
 *              | free? |------------|
 *              |_______|
 *                  |
 *                  |Yes
 *                  |
 *                  |
 *              ____________
 *              |           |
 *              | Wait difs |
 *              |___________|
 */
void CSMA_CONTROL::waitForTurn(){
    notFree:
    while (!this->checkChannel());

    //After this, should check if channel is free for difs
    //Since channel check taks slot amount of time, channel check time is multiple of slot time always
    //To cover for difs, assumes sifs is 2 slot times for now, close to ieee 802.11 a
    //This leads to difs being 4 slots

    for (size_t i = 0; i < 4; i++){
        if (!this->checkChannel()){
            goto notFree;
        }
        
    }
    
    
    while (this->backoffCount > 0){
        if(this->checkChannel())
            this->backoffCount -= 1;
        else
            goto notFree;
    }
    
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
