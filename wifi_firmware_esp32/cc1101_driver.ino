/**
 * Copyright (c) 2011 panStamp <contact@panstamp.com>
 * Copyright (c) 2016 Tyler Sommer <contact@tylersommer.pro>
 * 
 * This file is part of the CC1101 project.
 * 
 * CC1101 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 * 
 * CC1101 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with CC1101; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 
 * USA
 * 
 * Author: Daniel Berenguer
 * Creation date: 03/03/2011
 */

//#include "cc1101_driver.h"

/**
 * Macros
 */
// Select (SPI) CC1101
#define cc1101_Select()  digitalWrite(SS, LOW)
// Deselect (SPI) CC1101
#define cc1101_Deselect()  digitalWrite(SS, HIGH)
// Wait until SPI MISO line goes low
#define wait_Miso()  while(digitalRead(MISO)>0)
// Get GDO0 pin state
#define getGDO0state()  digitalRead(CC1101_GDO0)
// Get GDO2 pin state
#define getGDO2state()  digitalRead(CC1101_GDO2)
// Wait until GDO0 line goes high
#define wait_GDO0_high()  while(!getGDO0state())
#define wait_GDO0_high_test(){\
  int abcdefg = 0;\
  while(1){\
    if (getGDO0state()){\
      Serial.println("EEEe");\
      break;\
    }\
    abcdefg += 1;\
    if (abcdefg == 300){\
      Serial.println(F("STUCK"));\
    }\
  }\
}
// Wait until GDO0 line goes low
#define wait_GDO0_low()  while(getGDO0state())
// Wait until GDO2 line goes high
#define wait_GDO2_high()  while(!getGDO2state())
// Wait until GDO0 line goes low
#define wait_GDO2_low()  while(getGDO2state())

 /**
  * PATABLE
  */
//const byte paTable[8] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60};

/**
 * CC1101
 * 
 * Class constructor
 */
CC1101::CC1101(void)
{
  carrierFreq = CFREQ_868;
  channel = CC1101_DEFVAL_CHANNR;
  syncWord[0] = CC1101_DEFVAL_SYNC1;
  syncWord[1] = CC1101_DEFVAL_SYNC0;
  devAddress = CC1101_DEFVAL_ADDR;
  cca_threshold = CC1101_CCA_DEFAULT_THRESHOLD;
}

/**
 * wakeUp
 * 
 * Wake up CC1101 from Power Down state
 */
void CC1101::wakeUp(void)
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * writeReg
 * 
 * Write single register into the CC1101 IC via SPI
 * 
 * 'regAddr'  Register address
 * 'value'  Value to be writen
 */
void CC1101::writeReg(byte regAddr, byte value) 
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  SPI.transfer(regAddr);                // Send register address
  SPI.transfer(value);                  // Send value
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * writeBurstReg
 * 
 * Write multiple registers into the CC1101 IC via SPI
 * 
 * 'regAddr'  Register address
 * 'buffer' Data to be writen
 * 'len'  Data length
 */
void CC1101::writeBurstReg(byte regAddr, byte* buffer, byte len)
{
  byte addr, i;
  
  addr = regAddr | WRITE_BURST;         // Enable burst transfer
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  SPI.transfer(addr);                   // Send register address
  
  for(i=0 ; i<len ; i++)
    SPI.transfer(buffer[i]);            // Send value

  cc1101_Deselect();                    // Deselect CC1101  
}

/**
 * cmdStrobe
 * 
 * Send command strobe to the CC1101 IC via SPI
 * 
 * 'cmd'  Command strobe
 */     
void CC1101::cmdStrobe(byte cmd) 
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  SPI.transfer(cmd);                    // Send strobe command
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * readReg
 * 
 * Read CC1101 register via SPI
 * 
 * 'regAddr'  Register address
 * 'regType'  Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
 * 
 * Return:
 *  Data byte returned by the CC1101 IC
 */
byte CC1101::readReg(byte regAddr, byte regType)
{
  byte addr, val;

  addr = regAddr | regType;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  SPI.transfer(addr);                   // Send register address
  val = SPI.transfer(0x00);             // Read result
  cc1101_Deselect();                    // Deselect CC1101

  return val;
}

/**
 * readBurstReg
 * 
 * Read burst data from CC1101 via SPI
 * 
 * 'buffer' Buffer where to copy the result to
 * 'regAddr'  Register address
 * 'len'  Data length
 */
void CC1101::readBurstReg(byte * buffer, byte regAddr, byte len) 
{
  byte addr, i;
  
  addr = regAddr | READ_BURST;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  SPI.transfer(addr);                   // Send register address
  for(i=0 ; i<len ; i++)
    buffer[i] = SPI.transfer(0x00);     // Read result byte by byte
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * reset
 * 
 * Reset CC1101
 */
void CC1101::reset(void) 
{
  cc1101_Deselect();                    // Deselect CC1101
  delayMicroseconds(5);
  cc1101_Select();                      // Select CC1101
  delayMicroseconds(10);
  cc1101_Deselect();                    // Deselect CC1101
  delayMicroseconds(41);
  cc1101_Select();                      // Select CC1101

  wait_Miso();                          // Wait until MISO goes low
  SPI.transfer(CC1101_SRES);            // Send reset command strobe
  wait_Miso();                          // Wait until MISO goes low

  cc1101_Deselect();                    // Deselect CC1101

  setCCregs();                          // Reconfigure CC1101

  setRxState();
  //setMonitorCCA();
}

/**
 * setCCregs
 * 
 * Configure CC1101 registers
 */
void CC1101::setCCregs(void) 
{
  writeReg(CC1101_IOCFG2,  CC1101_DEFVAL_IOCFG2);
  writeReg(CC1101_IOCFG1,  CC1101_DEFVAL_IOCFG1);
  writeReg(CC1101_IOCFG0,  CC1101_DEFVAL_IOCFG0);
  writeReg(CC1101_FIFOTHR,  CC1101_DEFVAL_FIFOTHR);
  writeReg(CC1101_PKTLEN,  CC1101_DEFVAL_PKTLEN);
  writeReg(CC1101_PKTCTRL1,  CC1101_DEFVAL_PKTCTRL1);
  writeReg(CC1101_PKTCTRL0,  CC1101_DEFVAL_PKTCTRL0);

  // Set default synchronization word
  setSyncWord(syncWord);

  // Set default device address
  setDevAddress(devAddress);

  // Set default frequency channel
  setChannel(channel);
  
  writeReg(CC1101_FSCTRL1,  CC1101_DEFVAL_FSCTRL1);
  writeReg(CC1101_FSCTRL0,  CC1101_DEFVAL_FSCTRL0);

  // Set default carrier frequency = 868 MHz
  setCarrierFreq(carrierFreq);

  // RF speed
  if (workMode == MODE_LOW_SPEED)
    writeReg(CC1101_MDMCFG4,  CC1101_DEFVAL_MDMCFG4_4800);
  else
    writeReg(CC1101_MDMCFG4,  CC1101_DEFVAL_MDMCFG4_38400);
    
  writeReg(CC1101_MDMCFG3,  CC1101_DEFVAL_MDMCFG3);
  writeReg(CC1101_MDMCFG2,  CC1101_DEFVAL_MDMCFG2);
  writeReg(CC1101_MDMCFG1,  CC1101_DEFVAL_MDMCFG1);
  writeReg(CC1101_MDMCFG0,  CC1101_DEFVAL_MDMCFG0);
  writeReg(CC1101_DEVIATN,  CC1101_DEFVAL_DEVIATN);
  writeReg(CC1101_MCSM2,  CC1101_DEFVAL_MCSM2);
  writeReg(CC1101_MCSM1,  CC1101_DEFVAL_MCSM1);
  writeReg(CC1101_MCSM0,  CC1101_DEFVAL_MCSM0);
  writeReg(CC1101_FOCCFG,  CC1101_DEFVAL_FOCCFG);
  writeReg(CC1101_BSCFG,  CC1101_DEFVAL_BSCFG);

  writeReg(CC1101_AGCCTRL2,  CC1101_DEFVAL_AGCCTRL2);
  writeReg(CC1101_AGCCTRL1,  CC1101_DEFVAL_AGCCTRL1);
  writeReg(CC1101_AGCCTRL0,  CC1101_DEFVAL_AGCCTRL0);
  
  writeReg(CC1101_WOREVT1,  CC1101_DEFVAL_WOREVT1);
  writeReg(CC1101_WOREVT0,  CC1101_DEFVAL_WOREVT0);
  writeReg(CC1101_WORCTRL,  CC1101_DEFVAL_WORCTRL);
  writeReg(CC1101_FREND1,  CC1101_DEFVAL_FREND1);
  writeReg(CC1101_FREND0,  CC1101_DEFVAL_FREND0);
  writeReg(CC1101_FSCAL3,  CC1101_DEFVAL_FSCAL3);
  writeReg(CC1101_FSCAL2,  CC1101_DEFVAL_FSCAL2);
  writeReg(CC1101_FSCAL1,  CC1101_DEFVAL_FSCAL1);
  writeReg(CC1101_FSCAL0,  CC1101_DEFVAL_FSCAL0);
  writeReg(CC1101_RCCTRL1,  CC1101_DEFVAL_RCCTRL1);
  writeReg(CC1101_RCCTRL0,  CC1101_DEFVAL_RCCTRL0);
  writeReg(CC1101_FSTEST,  CC1101_DEFVAL_FSTEST);
  writeReg(CC1101_PTEST,  CC1101_DEFVAL_PTEST);
  writeReg(CC1101_AGCTEST,  CC1101_DEFVAL_AGCTEST);
  writeReg(CC1101_TEST2,  CC1101_DEFVAL_TEST2);
  writeReg(CC1101_TEST1,  CC1101_DEFVAL_TEST1);
  writeReg(CC1101_TEST0,  CC1101_DEFVAL_TEST0);

  //Serial.printf("state after leaving: %d\n", (int) radio.readStatusReg(CC1101_MARCSTATE));
}

/**
 * init
 * 
 * Initialize CC1101 radio
 *
 * @param freq Carrier frequency
 * @param mode Working mode (speed, ...)
 */
void CC1101::init(uint8_t freq, uint8_t mode)
{
  carrierFreq = freq;
  workMode = mode;
 #ifdef ESP32
  pinMode(SS, OUTPUT);                       // Make sure that the SS Pin is declared as an Output
 #endif
  SPI.begin();                          // Initialize SPI interface

  // Set the speed of the SPI interface. Ideally, this should be as 
  // high as possible to avoid time overheads. According to the 
  // CC1101 datasheet, up to 10 MHz should be achievable, but empirical
  // testing suggests that is not possible on the current setup. Perhaps
  // shorter cables would allow for higher speeds.
  SPI.beginTransaction(SPISettings(1500000, MSBFIRST, SPI_MODE0));
  pinMode(CC1101_GDO0, INPUT);          // Config GDO0 as input
  pinMode(CC1101_GDO2, INPUT);          // Config GDO2 as input

  reset();                              // Reset CC1101

  // Configure PATABLE
  setTxPowerAmp(PA_LowPower);

}

/**
 * setSyncWord
 * 
 * Set synchronization word
 * 
 * 'syncH'  Synchronization word - High byte
 * 'syncL'  Synchronization word - Low byte
 */
void CC1101::setSyncWord(uint8_t syncH, uint8_t syncL) 
{
  writeReg(CC1101_SYNC1, syncH);
  writeReg(CC1101_SYNC0, syncL);
  syncWord[0] = syncH;
  syncWord[1] = syncL;
}

/**
 * setSyncWord (overriding method)
 * 
 * Set synchronization word
 * 
 * 'syncH'  Synchronization word - pointer to 2-byte array
 */
void CC1101::setSyncWord(byte *sync) 
{
  CC1101::setSyncWord(sync[0], sync[1]);
}

/**
 * setDevAddress
 * 
 * Set device address
 * 
 * @param addr  Device address
 */
void CC1101::setDevAddress(byte addr) 
{
  writeReg(CC1101_ADDR, addr);
  devAddress = addr;
}

/**
 * setChannel
 * 
 * Set frequency channel
 * 
 * 'chnl' Frequency channel
 */
void CC1101::setChannel(byte chnl) 
{
  writeReg(CC1101_CHANNR,  chnl);
  channel = chnl;
}

/**
 * setCarrierFreq
 * 
 * Set carrier frequency
 * 
 * 'freq' New carrier frequency
 */
void CC1101::setCarrierFreq(byte freq)
{
  switch(freq)
  {
    case CFREQ_915:
      writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_915);
      writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_915);
      writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_915);
      break;
    case CFREQ_433:
      writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_433);
      writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_433);
      writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_433);
      break;
    case CFREQ_918:
      writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_918);
      writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_918);
      writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_918);
      break;
    default:
      writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_868);
      writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_868);
      writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_868);
      break;
  }
   
  carrierFreq = freq;  
}

/**
 * setPowerDownState
 * 
 * Put CC1101 into power-down state
 */
void CC1101::setPowerDownState() 
{
  // Comming from RX state, we need to enter the IDLE state first
  cmdStrobe(CC1101_SIDLE);
  // Enter Power-down state
  cmdStrobe(CC1101_SPWD);
}


/**
 * sendData
 * 
 * Send data packet via RF
 * 
 * 'packet' Packet to be transmitted. First byte is the destination address
 * 
 * Empirical testing on the ESP32 shows an overhead of ~1.7 ms until
 * transmission starts (i.e., until the preamble and sync words are sent).
 *
 *  Return:
 *    True if the transmission succeeds
 *    False otherwise
 */
bool CC1101::sendData(CCPACKET packet)
{
  unsigned short remainingBytes = packet.length;
    
  // The radio can only (automatically) handle chunks of up to 64 bytes.
  // Compute the length of the last chunk and load it on the PKTLEN register.
  // Note: the +2 below is to account for the size of the packet length header
  // field.
  writeReg(CC1101_PKTLEN, (packet.length + 2) % 256);

  // Push the packet length to the TX FIFO buffer
  writeBurstReg(CC1101_TXFIFO, (byte *) & (packet.length), sizeof(packet.length));

  // For small packets (< 255 bytes), we need to change the transmission mode
  // to fixed length right here, otherwise it will not be changed in the main
  // transmission loop.
  if (packet.length < 255) setPacketLengthConfig(CC1101_LENGTH_CONFIG_FIXED);
  
  // Enable monitoring the TX FIFO through GDO2
  setMonitorTxFifo();

  // Put the radio into the TX state
  setTxState();

  // Wait until the transmission starts
  wait_GDO0_high();

  // As space becomes available in the FIFO, push more
  // of the packet payload.
  while (remainingBytes > 0) {

    // Wait until there is data available on the FIFO
    while (getGDO2state()) {
      if (!getGDO0state() && getGDO2state()) {

        Serial.println("Premature end to the transmission!");     
  
        // Clean-up
        setIdleState();       // Enter IDLE state
        flushTxFifo();        // Flush Tx FIFO
      
        // Put the radio back in infinite packet mode  
        setPacketLengthConfig(CC1101_LENGTH_CONFIG_INFINITE);
      
        // Enter back into RX state
        setRxState();
        
        return false;
      }
    }
    
    if (remainingBytes == 8) {

      // If less than 256 bytes remain, put the radio into 
      // fixed length mode.
      setPacketLengthConfig(CC1101_LENGTH_CONFIG_FIXED);
    }

    // Push the next byte into the FIFO
    writeReg(CC1101_TXFIFO, packet.data[packet.length - remainingBytes]);
    remainingBytes--;
  }

  // Wait until the end of the packet transmission
  wait_GDO0_low();

  setIdleState();       // Enter IDLE state
  flushTxFifo();        // Flush Tx FIFO

  // Put the radio back in infinite packet mode  
  setPacketLengthConfig(CC1101_LENGTH_CONFIG_INFINITE);

  // Enter back into RX state
  setRxState();
  //setMonitorCCA();

  return true;
}

/**
 * receiveData
 * 
 * Read data packet from RX FIFO
 *
 * 'packet' Container for the packet received
 * 
 * Return:
 *  Amount of bytes received
 */
unsigned short CC1101::receiveData(CCPACKET * packet)
{
  byte val;
  unsigned short remainingBytes;

  // Enable monitoring the RX FIFO through GDO2
  setMonitorRxFifo();

  // Wait for enough bytes on the RX FIFO
  wait_GDO2_high();
  Serial.println(F("AAAAAAAAAAAAA"));
  
  // Read the packet size
  readBurstReg((byte *) & (packet->length), CC1101_RXFIFO, sizeof(packet->length));
  remainingBytes = packet->length;

  // The radio can only (automatically) handle chunks of up to 64 bytes.
  // Compute the length of the last chunk and load it on the PKTLEN register.
  // Note: the +2 below is to account for the size of the packet length header
  // field.
  writeReg(CC1101_PKTLEN, (packet->length + 2) % 256);
  
  // For small packets (< 255 bytes), we need to change the transmission mode
  // to fixed length right here, otherwise it will not be changed in the main
  // transmission loop.
  if (packet->length < 255) setPacketLengthConfig(CC1101_LENGTH_CONFIG_FIXED);
      
  // Read the rest of the packet.
  // As space becomes available in the FIFO, pull more
  // of the packet payload.
  while (remainingBytes > 0) {
    
    // Wait until there is data available on the FIFO
    while (!getGDO2state()) {
      if (!getGDO0state() && !getGDO2state()) {
        setIdleState();       // Enter IDLE state
        flushRxFifo();        // Flush Rx FIFO
        
        // Put the radio back in infinite packet mode  
        setPacketLengthConfig(CC1101_LENGTH_CONFIG_INFINITE);
        
        // Back to RX state
        setRxState();
        return 0;
      }
    }
  
    if (remainingBytes == 255) {

      // If less than 256 bytes remain, put the radio into 
      // fixed length mode.
      setPacketLengthConfig(CC1101_LENGTH_CONFIG_FIXED);
    }

    // Pull the next byte from the FIFO, but first check if there is not
    // going to be a buffer overflow.
    if (packet->length - remainingBytes >= CCPACKET_DATA_LEN) {

      // The packet is too large. Abort.
      //Serial.println("Aborting reception...");
      setIdleState();       // Enter IDLE state
      flushRxFifo();        // Flush Rx FIFO
      
      // Put the radio back in infinite packet mode  
      setPacketLengthConfig(CC1101_LENGTH_CONFIG_INFINITE);
      
      // Back to RX state
      setRxState();
      return 0;
    }
    packet->data[packet->length - remainingBytes] = readConfigReg(CC1101_RXFIFO);
    remainingBytes--;
  }  
    
  // Read the packet metadata
  // Read RSSI
  wait_GDO2_high();
  packet->rssi = readConfigReg(CC1101_RXFIFO);
  // Read LQI and CRC_OK
  wait_GDO2_high();
  val = readConfigReg(CC1101_RXFIFO);
  packet->lqi = val & 0x7F;
  packet->crc_ok = bitRead(val, 7);
  
  setIdleState();       // Enter IDLE state
  flushRxFifo();        // Flush Rx FIFO
  
  // Put the radio back in infinite packet mode  
  setPacketLengthConfig(CC1101_LENGTH_CONFIG_INFINITE);
  
  // Back to RX state
  setRxState();

  return packet->length;
}

/**
 * cca
 * 
 * determines the channel state based on the RSSI reported 
 * by the radio. This function assumes that the radio is on
 * RX state.
 * 
 * Notice that the radio measures the RSSI with a frequency
 * that is dependent on (Sec. 17.3 of the datasheet):
 *  - The crystal oscillator
 *  - The configured reception bandwidth.
 *  - The filter length.
 *  
 * For the default configuration of this library, this results
 * in the value being updated every ~25 us. So, there is no
 * point calling this function more frequently than that.
 * 
 * Return:
 *  true (channel is clear) or false (channel is busy)
 */
bool CC1101::cca(void) 
{
  //return getGDO2state();

  
  while(1) {

    byte marcState = radio.readStatusReg(CC1101_MARCSTATE);

    if (marcState == 17) {

      // Radio got into RXFIFOOVERFLOW state (because
      // some packet was detected by the radio but not
      // processed by the MCU). We need to go back to 
      // the RX state so that valid RSSI measurements 
      // are available again.
      
      radio.setIdleState();       // Enter IDLE state
      radio.flushRxFifo();        // Flush Rx FIFO
      radio.setRxState();         // Back to RX state

       continue ;
    }

    return raw2rssi(radio.readStatusReg(CC1101_RSSI)) < cca_threshold;
  }
  
}


/**
 * setRxState
 * 
 * Enter Rx state
 */
void CC1101::setRxState(void)
{
  cmdStrobe(CC1101_SRX);

  //wait for state transition from idle to rx from receive
  while(radio.readStatusReg(CC1101_MARCSTATE) != RFSTATE_RX);
}

/**
 * setTxState
 * 
 * Enter Tx state
 */
void CC1101::setTxState(void)
{
  cmdStrobe(CC1101_STX);
}

/**
 * setPacketLengthConfig
 * 
 * Set the LENGTH_CONFIG portion of the 
 *  PKTCTRL0 – Packet Automation Control 
 *  register. This allows changing the
 *  transmission mode from fixed length,
 *  variable length or infinit.
 */
void CC1101::setPacketLengthConfig(int mode)
{
  // Read the current config value
  uint8_t regVal = readConfigReg(CC1101_PKTCTRL0);
  // The LENGTH config corresponds to the lower two bits.
  regVal = regVal & 0xFC | mode;
  // Update the configuration
  writeReg(CC1101_PKTCTRL0,  regVal);
}

/**
 * jamming
 * 
 * Creates a jammer by continuously transmitting an
 * infinite packet. Warning: use this for testing only!
 */
void CC1101::jamming(void)
{
  unsigned short len = 0xFFFF;
  byte dummyData = 0;
  int count = 0;

  // Push the packet length to the TX FIFO buffer
  writeBurstReg(CC1101_TXFIFO, (byte *) & len, sizeof(len));

  // Enable monitoring the TX FIFO through GDO2
  setMonitorTxFifo();

  Serial.printf("macrstate = %d\r\n",  radio.readStatusReg(CC1101_MARCSTATE));

  // Put the radio into the TX state
  setTxState();

  // Wait until the transmission starts
  wait_GDO0_high();

  // As space becomes available in the FIFO, push more
  // of the packet payload.
  while (1) {

    if (count++ % 1000 == 0) Serial.printf("macrstate = %d\r\n",  radio.readStatusReg(CC1101_MARCSTATE));
    
    // Wait until there is data available on the FIFO
    while (getGDO2state()) {
      if (!getGDO0state() && getGDO2state()) {

        // This should not happen.
        
        Serial.println("Jamming failed!");     
  
        // Clean-up
        setIdleState();       // Enter IDLE state
        flushTxFifo();        // Flush Tx FIFO
      
        // Put the radio back in infinite packet mode  
        setPacketLengthConfig(CC1101_LENGTH_CONFIG_INFINITE);
      
        // Enter back into RX state
        setRxState();
      }
    }

    // Push the next byte into the FIFO
    writeReg(CC1101_TXFIFO, dummyData);
  }

}


/**
 * raw2rssi
 * 
 * converts the raw RSSI values reported by the radio
 * to dBm.
 * See: http://www.ti.com/lit/an/swra114d/swra114d.pdf
 * 
 * Return:
 *  the RSSI in dBm
 */
int CC1101::raw2rssi(char raw) {
  
    uint8_t rssi_dec;
    // TODO: This rssi_offset is dependent on baud and MHz; this is for 38.4kbps and 433 MHz.
    uint8_t rssi_offset = 74;
    
    rssi_dec = (uint8_t) raw;
    if (rssi_dec >= 128)
        return ((int)( rssi_dec - 256) / 2) - rssi_offset;
    else
        return (rssi_dec / 2) - rssi_offset;
}

/**
 * raw2lqi
 * 
 * converts the raw LQI values reported by the radio.
 * 
 * Return:
 *  the LQI.
 */
int CC1101::raw2lqi(char raw) {
  
    return 0x3F - raw;
}
