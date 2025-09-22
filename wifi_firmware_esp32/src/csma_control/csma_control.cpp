#include "csma_control.h"
#include "interframe_spaces.h"
#include "cc1101.h"

#include "esp32-hal.h" // micros

CSMA_CONTROL::CSMA_CONTROL(CONTENTION_BACKOFF *contentionBackoff)
{
    this->contentionAlgorithm = contentionBackoff;

#ifndef CCA_FROM_GDO2_PIN
    unsigned long start = micros();
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
        if (CC1101::radio->cca())
        {
            break;
        }
    };

    start = micros();
    while (true)
    {
        if (!CC1101::radio->cca())
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
        if (!CC1101::radio->cca())
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
    while (!CC1101.radio->cca())
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

bool CSMA_CONTROL::difsCheck()
{

    for (uint8_t k = 0; k < this->sifsRepetition + this->backoffRepetition; k += 1)
    {
        if (!this->checkChannel())
            return false;
    }

    return true;
}

bool CSMA_CONTROL::backoffCheck()
{
    for (uint8_t k = 0; k < this->backoffRepetition; k += 1)
    {
        if (!this->checkChannel())
            return false;
    }

    return true;
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
