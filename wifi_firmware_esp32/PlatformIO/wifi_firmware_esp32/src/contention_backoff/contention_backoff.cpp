#include "contention_backoff.h"
#include "stdlib.h"

uint16_t CONTENTION_BACKOFF::getBackoff()
{
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/random.html#_CPPv410esp_randomv
  // https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
  // minimum as 0

  int random = (int)rand();

  if (random < 0)
    random = -random;

  return random / (RAND_MAX / (contentionWindow + 1) + 1);
}

void MILD_BACKOFF::reduceContentionWindow()
{
  uint16_t newWindow = this->contentionWindow - 1;
  if (newWindow >= this->minimum)
  {
    this->contentionWindow = newWindow;
  }
}

void MILD_BACKOFF::increaseContentionWindow()
{
  uint16_t newWindow = this->contentionWindow << 1;
  if (newWindow <= this->maximum)
  {
    this->contentionWindow = newWindow;
  }
}

MILD_BACKOFF::MILD_BACKOFF()
{
  this->minimum = 15;
  this->contentionWindow = 15;
  this->maximum = 1023;
}

void LINEAR_BACKOFF::reduceContentionWindow()
{
  uint16_t newWindow = this->contentionWindow - 1;
  if (newWindow >= this->minimum)
  {
    this->contentionWindow = newWindow;
  }
}

void LINEAR_BACKOFF::increaseContentionWindow()
{
  uint16_t newWindow = this->contentionWindow + 1;
  if (newWindow <= this->maximum)
  {
    this->contentionWindow = newWindow;
  }
}
LINEAR_BACKOFF::LINEAR_BACKOFF()
{
  this->minimum = 15;
  this->contentionWindow = 15;
  this->maximum = 1027;
}

void CONSTANT_BACKOFF::reduceContentionWindow() {}

void CONSTANT_BACKOFF::increaseContentionWindow() {}

CONSTANT_BACKOFF::CONSTANT_BACKOFF()
{
  this->minimum = 100;
  this->contentionWindow = 100;
  this->maximum = 100;
}

void NO_BACKOFF::reduceContentionWindow() {}

void NO_BACKOFF::increaseContentionWindow() {}

NO_BACKOFF::NO_BACKOFF()
{
  this->minimum = 0;
  this->contentionWindow = 0;
  this->maximum = 0;
}

/**
 * Return protocol instance from identifier.
 *  If param not recognized returns constant instance
 */
CONTENTION_BACKOFF* CONTENTION_BACKOFF::getBackoffProtocol(BACKOFF_PROTOCOLS protocol_enum)
{
  switch (protocol_enum)
  {
  case MILD:
    return new MILD_BACKOFF();
  case LINEAR:
    return new LINEAR_BACKOFF();
  case NON_EXISTANT:
    return new NO_BACKOFF();
  case CONSTANT:
  default:
    return new CONSTANT_BACKOFF();
  }
}