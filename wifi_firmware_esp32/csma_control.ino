CSMA_CONTROL::CSMA_CONTROL(bool (*isChannelFree)(), CONTENTION_BACKOFF* contentionBackoff){
    this->checkChannel = isChannelFree;
    this->contentionAlgorithm = contentionBackoff;

    unsigned long start = micros();
    this->checkChannel();
    unsigned long end = micros();

    unsigned long duration = end - start;

    this->backoffRepetition = (uint8_t) (BACKOFF_TIME_SLOT / duration) + 1;
    this->sifsRepetition = (uint8_t) (SIFS / duration) + 1;

    this->backoffCount = this->contentionAlgorithm->getBackoff();
    
}
CSMA_CONTROL::~CSMA_CONTROL(){
    delete contentionAlgorithm;
}

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
        this->contentionAlgorithm->reduceContentionWindow();
    }
    else{
        this->contentionAlgorithm->increaseContentionWindow();
    }
    
    this->backoffCount = this->contentionAlgorithm->getBackoff();

}
