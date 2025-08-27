void receiveAndAnswerTask(void* unused_param){

  while(true){

    vTaskSuspend(NULL);  //suspend self; done on activation and after each receive

    if(!receiver()){
      continue;
    }
    
    PRINT("DST: ");
    PRINTLN_MAC(receiveFrame->addr_dest);

    //Set destination address
    memcpy( answerFrame->addr_dest, receiveFrame->addr_src, MAC_ADDRESS_SIZE );

    //PRINTLN("Received frame on response task");
    
    if(PACKET_IS_DATA(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_ACK(answerFrame);
      answerFrame->duration = 0; // no more to send after this, since fragmentation is not supported
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);


      PRINTLN("SENT ACK");
    }
    else if (PACKET_IS_RTS(receiveFrame)){
      detachInterrupt(CC1101_GDO0);

      PACKET_TO_CTS(answerFrame);
      answerFrame->duration = receiveFrame->duration - SIFS - radio.transmittionTime(packet_to_receive); 
      //Warning; This really counts on the packet sent not being interrupted, and therefore causing its failure
      //Creating prints in this step to check if the packet was sent or not should not cause any grand issues during testing;
      if(!radio.sendData(answer_packet)){
        PRINTLN("Response failed, assumed task was interrupted");
      }

      attachInterrupt(CC1101_GDO0, messageReceived, RISING);

      PRINTLN("SENT CTS");
    }
    else{
      PRINT("Response task, frame control not recognized, with value: ");
      PRINTLN_VALUE(receiveFrame->frame_control[0]);
    }

  }
      
}


void changeParametersTask(void* unusedParam){

  //Implementing queue solution for now

  PRINTLN("Created change parameters task");

  uint8_t params_buffer[sizeof(macProtocolParameters)];

  macProtocolParameters *params = (macProtocolParameters*) params_buffer;

  while(true){

    /**
     * check if portMAX_DELAY wait is forever
    */
    if(xQueueReceive(protocolParametersQueueHandle, params_buffer, portMAX_DELAY) == pdFALSE){
      continue;
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setIdleState(); // to avoid rx overflow
    PRINTLN("Set to idle state, avoiding answer task being activated");
#endif

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    PRINTLN("\n\nCHANGE PARAMETERS GOT EXCLUSIVITY\n\n");

    mac_data.retries = 0; mac_data.failures = 0; mac_data.successes = 0; mac_data.startTime = millis();

    if(params->csma_contrl_params.used){
      delete csma_control;
      csma_control = new CSMA_CONTROL(&checkChannel, getBackoffProtocol(params->csma_contrl_params.backoff_protocol));
    }


    if(params->traf_gen_time.used){
      trf_gen->setTime(params->traf_gen_time.time_mode, params->traf_gen_time.waiting_time);
    }

    if(params->traf_gen_addr.used){
      PRINT("CHANGE PARAMS TASK NEW DEST MAC: "); PRINTLN_MAC(params->traf_gen_addr.address);
      trf_gen->setDestAddress(params->traf_gen_addr.address);
      memcpy(rtsFrame->addr_dest, params->traf_gen_addr.address, MAC_ADDRESS_SIZE);
    }

    if(params->traf_gen_data.used){
      Serial.println("Changing message");

      rtsFrame->duration = durationCalculation(params->traf_gen_data.message_length);
      trf_gen->setMessage(
        params->traf_gen_data.message, 
        params->traf_gen_data.message_length
      );

      //TODO find better solution than malloc and free if possible
      free(params->traf_gen_data.message);
      
    }

#ifdef ANSWER_TASK_CHANGES_WITH_PARAMETERS
    radio.setRxState();
    PRINTLN("On rx state, allowing answer task activating");
#endif

    xSemaphoreGive(xSemaphore);
    PRINTLN("CHANGED, delay\n");

  }

}