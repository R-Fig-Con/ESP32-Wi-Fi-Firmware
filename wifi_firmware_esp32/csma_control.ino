CSMA_CONTROL::CSMA_CONTROL(bool (*isChannelFree)(), CONTENTION_BACKOFF *contentionBackoff)
{
    this->checkChannel = isChannelFree;
    this->contentionAlgorithm = contentionBackoff;

#ifndef CCA_FROM_GDO2_PIN
    unsigned long start = micros();
    this->checkChannel();
    unsigned long end = micros();

    unsigned long duration = end - start;

    this->backoffRepetition = (uint8_t)(BACKOFF_TIME_SLOT / duration) + 1;
    this->sifsRepetition = (uint8_t)(SIFS / duration) + 1;

#endif

    this->backoffCount = this->contentionAlgorithm->getBackoff();
}
CSMA_CONTROL::~CSMA_CONTROL()
{
    delete this->contentionAlgorithm;
}

#ifdef CCA_FROM_GDO2_PIN
void CSMA_CONTROL::waitForTurn()
{

    unsigned long start;
notFree:
    while (true)
    {
        if (checkChannel())
        {
            break;
        }
    };

    start = micros();
    while (true)
    {
        if (!this->checkChannel())
        {
            goto notFree;
        }

        if (micros() - start >= DIFS)
        {
            break;
        }
    }

    // backoff time slots
    start = micros();
    while (true)
    {
        if (!this->checkChannel())
        {
            goto notFree;
        }

        if (micros() - start >= BACKOFF_TIME_SLOT)
        {
            this->backoffCount -= 1;

            if (this->backoffCount == 0)
            {
                break;
            }
        }
    }
}
#else
void CSMA_CONTROL::waitForTurn()
{
notFree:
    while (!checkChannel())
        ;

    if (!difsCheck())
    {
        goto notFree;
    }

    while (this->backoffCount > 0)
    {
        if (backoffCheck())
            this->backoffCount -= 1;
        else
            goto notFree;
    }
}
#endif

void CSMA_CONTROL::ackReceived(bool wasReceived)
{
    if (wasReceived)
    {
        this->contentionAlgorithm->reduceContentionWindow();
    }
    else
    {
        this->contentionAlgorithm->increaseContentionWindow();
    }

    this->backoffCount = this->contentionAlgorithm->getBackoff();
}
