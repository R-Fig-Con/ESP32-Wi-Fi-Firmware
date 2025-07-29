uint16_t CONTENTION_BACKOFF::getBackoff(){
    //https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/random.html#_CPPv410esp_randomv
    //https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
    //minimum as 0

    int random = (int) rand();

    if(random < 0) random = -random;

    return random / (RAND_MAX / (contentionWindow + 1) + 1);
}


/**
 * multiple increase, linear decrease
*/
class MILD_BACKOFF: public CONTENTION_BACKOFF{

  void reduceContentionWindow(){
    uint16_t newWindow = this->contentionWindow - 1;

    if (newWindow >= this->minimum){
      this->contentionWindow = newWindow;
    }
  }

  
  void increaseContentionWindow(){
    uint16_t newWindow = this->contentionWindow << 1;

    if (newWindow <= this->maximum){
      this->contentionWindow = newWindow;
    }
  }

  public:
     MILD_BACKOFF(){
      this->minimum = 15;
      this->contentionWindow = 15;
      this->maximum = 1023;
     }

};


class LINEAR_BACKOFF: public CONTENTION_BACKOFF{
  void reduceContentionWindow(){
    uint16_t newWindow = this->contentionWindow - 1;

    if (newWindow >= this->minimum){
      this->contentionWindow = newWindow;
    }
  }

  
  void increaseContentionWindow(){
    uint16_t newWindow = this->contentionWindow + 1;

    if (newWindow <= this->maximum){
      this->contentionWindow = newWindow;
    }
  }

  public:
     LINEAR_BACKOFF(){
      this->minimum = 15;
      this->contentionWindow = 15;
      this->maximum = 1027;
     }
};


/**
 * Constantly on value 15
 */
class CONSTANT_BACKOFF: public CONTENTION_BACKOFF{
  void reduceContentionWindow(){}

  
  void increaseContentionWindow(){}

  public:
     CONSTANT_BACKOFF(){
      this->minimum = 100;
      this->contentionWindow = 100;
      this->maximum = 100;
     }
};

/**
 * great to check collision when time in both traffics is the same and linear
 * 
 * Could be good to use in demonstration
 */
class NO_BACKOFF: public CONTENTION_BACKOFF{
  void reduceContentionWindow(){}

  
  void increaseContentionWindow(){}

  public:
    NO_BACKOFF(){
      this->minimum = 0;
      this->contentionWindow = 0;
      this->maximum = 0;
    }
};

CONTENTION_BACKOFF* getBackoffProtocol(BACKOFF_PROTOCOLS protocol_enum){
  switch(protocol_enum){
    case MILD:
      return new MILD_BACKOFF();
    case LINEAR:
      return new LINEAR_BACKOFF();
    case NON_EXISTANT:
      return new NO_BACKOFF();
    case CONSTANT:
      return new CONSTANT_BACKOFF();
  }
}