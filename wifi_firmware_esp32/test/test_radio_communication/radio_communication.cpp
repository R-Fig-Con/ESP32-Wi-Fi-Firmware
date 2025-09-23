#include <Arduino.h>
#include <unity.h>
#include "esp_system.h"
#include <cc1101.h>
#include <mac_data.h>

byte syncWord[2] = {199, 10};
volatile bool packetWaiting;

void messageReceived()
{
    packetWaiting = true;
}

/**
 * no setup needed, exists to be modified when receiving
 */
CCPACKET packet_to_receive;

/**
 * Directly affects 'packet_to_receive' global var
 * returns crc_ok
 */
bool receiver()
{

    // Yes. Disable the reception interruption while we handle this packet.
    detachInterrupt(CC1101_GDO0);

    // Try to receive the packet
    CC1101::radio->receiveData(&packet_to_receive);
    packetWaiting = false;

    if (packet_to_receive.crc_ok == false)
    {
        return false;
    }

    attachInterrupt(CC1101_GDO0, messageReceived, RISING);

    return true;
}

static char hello_world[] = "Hello World!";

CCPACKET packet_to_send;

/**
 * prepares packet_to_send
 */
void sender_create_data_packet()
{

    memcpy(packet_to_send.data, hello_world, strlen(hello_world)); // todo check about \0 size

    packet_to_send.length = strlen(hello_world);
}

CCPACKET answer_packet;

static char answer_text[] = "Received packet";
void receiver_create_data_packet()
{
    memcpy(answer_packet.data, answer_text, strlen(answer_text));
    answer_packet.length = strlen(answer_text);
}

/**
 * sends packet, waits for ack. Ack is not actually processed
 */
void communication_sender_test(void)
{
    CC1101::radio->sendData(packet_to_send);
    unsigned long wait_start = micros();

    attachInterrupt(CC1101_GDO0, messageReceived, RISING);

    bool give_up = false;

    while (!packetWaiting)
    {
        if (micros() - wait_start >= 4000)
        {
            give_up = true;
            goto end;
        }
    }

    receiver();



end:

    // radio configuration
    // unecessary if is ran only once as is in setup
    detachInterrupt(CC1101_GDO0);

    if (give_up)
    {
        TEST_FAIL_MESSAGE("Failed to receive answer; aborted after 4000 microsseconds");
    }

    TEST_ASSERT_EQUAL_STRING(answer_text, packet_to_receive.data);
}

/**
 * waits for data, sends ack
 *
 * Prints confirm payload of data, crc not ok message
 */
void communication_receiver_test(void)
{
    attachInterrupt(CC1101_GDO0, messageReceived, RISING);
    CC1101::radio->setRxState();

    while (!packetWaiting)
        ;
    if (receiver())
    {
        TEST_ASSERT_EQUAL_STRING(hello_world, packet_to_receive.data);
        CC1101::radio->sendData(answer_packet);
    }
    else
    {
        TEST_FAIL_MESSAGE("Radio crc failure. Attempt to rerun test");
    }
}

String role = "undefined";

/**
 * Setup  starts serial communication
 */
void setup()
{

    CC1101::radio->init();
    CC1101::radio->setSyncWord(syncWord);
    CC1101::radio->setCarrierFreq(CFREQ_433);
    CC1101::radio->disableAddressCheck();
    CC1101::radio->setTxPowerAmp(PA_LowPower);

    delay(1000);

    // Serial communication for debug
    Serial.begin(57600);

    delay(1000);

    receiver_create_data_packet();
    sender_create_data_packet();

    UNITY_BEGIN();

    while (Serial.available() == 0)
        ;

    role = Serial.readStringUntil('\n');

    if (role == "sender")
    {
        RUN_TEST(communication_sender_test);
    }
    else if (role == "receiver")
    {
        RUN_TEST(communication_receiver_test);
    }
    else
    {
        Serial.println("[Error] Unknown role");
    }

    UNITY_END();
}

void loop()
{
}
