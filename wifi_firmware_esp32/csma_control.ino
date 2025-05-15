//https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
int random(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

CSMA_CONTROL::CSMA_CONTROL(bool (*isChannelFree)()){
    this->checkChannel = isChannelFree;

    unsigned long start = micros();
    this->checkChannel();
    unsigned long end = micros();

    unsigned long duration = end - start;

    this->backoffRepetition = (uint8_t) (duration / BACKOFF_TIME_SLOT) + 1;
    this->sifsRepetition = (uint8_t) (duration / SIFS) + 1;

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
    while (!checkChannel());

    
    if (!difsCheck()){
        goto notFree;
    }
    
    
    while (this->backoffCount > 0){
        if(backoffCheck())
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
