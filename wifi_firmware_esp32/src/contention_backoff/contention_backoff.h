#ifndef CONTENTION_BACKOFF_H
#define CONTENTION_BACKOFF_H
#include "stdint.h"


/**
 * list of backoff protocols from which to choose
 */
enum BACKOFF_PROTOCOLS
{
     MILD,
     LINEAR,
     NON_EXISTANT,
     CONSTANT
};

class CONTENTION_BACKOFF
{
protected:
     /**
      * minimum value contention window allows
      */
     uint16_t minimum;

     /**
      * current contention window
      */
     uint16_t contentionWindow;

     /**
      * max value contention window allows
      */
     uint16_t maximum;

public:
     static CONTENTION_BACKOFF *getBackoffProtocol(BACKOFF_PROTOCOLS protocol_enum);
     /**
      * returns random backoff value
      */
     uint16_t getBackoff();

     /**
      * reduce contention window as desired by implementer
      */
     virtual void reduceContentionWindow();

     /**
      * increase contention window as desired by implementer
      */
     virtual void increaseContentionWindow();

     // virtual destructor
     virtual ~CONTENTION_BACKOFF() {}
};

class LINEAR_BACKOFF : public CONTENTION_BACKOFF
{
public:
     LINEAR_BACKOFF();

     virtual void reduceContentionWindow();

     virtual void increaseContentionWindow();
};

class CONSTANT_BACKOFF : public CONTENTION_BACKOFF
{
public:
     CONSTANT_BACKOFF();

     virtual void reduceContentionWindow();

     virtual void increaseContentionWindow();
};

class NO_BACKOFF : public CONTENTION_BACKOFF
{
public:
     NO_BACKOFF();

     virtual void reduceContentionWindow();

     virtual void increaseContentionWindow();
};

class MILD_BACKOFF : public CONTENTION_BACKOFF
{
public:
     MILD_BACKOFF();

     virtual void reduceContentionWindow();

     virtual void increaseContentionWindow();
};

#endif
