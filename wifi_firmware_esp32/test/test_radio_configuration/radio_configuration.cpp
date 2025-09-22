#include <Arduino.h>
#include <unity.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include <cc1101.h>

byte syncWord[2] = {199, 10};

void test_check_partnum_reg(void)
{
    TEST_ASSERT_EQUAL(0, CC1101::radio->readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER));
}

void test_check_version_reg(void){
    TEST_ASSERT_EQUAL(20, CC1101::radio->readReg(CC1101_VERSION, CC1101_STATUS_REGISTER));
}

void test_check_idle_state_reg(void){
    TEST_ASSERT_EQUAL(1, CC1101::radio->readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);
}

void test_check_freq_registers(void){
    TEST_ASSERT_EQUAL(16, CC1101::radio->readReg(CC1101_FREQ2, CC1101_CONFIG_REGISTER));

    TEST_ASSERT_EQUAL(167, CC1101::radio->readReg(CC1101_FREQ1, CC1101_CONFIG_REGISTER));

    TEST_ASSERT_EQUAL(98, CC1101::radio->readReg(CC1101_FREQ0, CC1101_CONFIG_REGISTER));
}

void test_check_receive_state(void){
    TEST_ASSERT_EQUAL(13, CC1101::radio->readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);
}

/**
 * Setup  starts serial communication
 */
void setup()
{
    //delay from https://github.com/platformio/platformio-examples/blob/develop/unit-testing/calculator/test/test_embedded/test_calculator.cpp
    //todo check if necessary
    delay(2000);

    CC1101::radio->init();
    CC1101::radio->setSyncWord(syncWord);
    CC1101::radio->setCarrierFreq(CFREQ_433);
    CC1101::radio->disableAddressCheck();
    CC1101::radio->setTxPowerAmp(PA_LowPower);

    delay(1000);

    UNITY_BEGIN();
    RUN_TEST(test_check_partnum_reg);
    RUN_TEST(test_check_version_reg);
    RUN_TEST(test_check_idle_state_reg);
    RUN_TEST(test_check_freq_registers);

    CC1101::radio->setRxState();

    RUN_TEST(test_check_receive_state);

    UNITY_END();
    
}

void loop()
{
}
