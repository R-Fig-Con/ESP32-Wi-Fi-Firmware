#include "traffic_generator.h"
#include "string.h"
#include "esp32-hal.h"

TRAFFIC_GEN::TRAFFIC_GEN(SEND_PROTOCOL *protocol, uint8_t my_addr[6], uint8_t destination_addr[6], uint16_t duration, uint16_t data_length)
{

    this->running = false;
    this->send_protocol = protocol;

    ieeeFrame *trf_frame = (ieeeFrame *)this->packet.data;
    PACKET_TO_DATA(trf_frame);

    memcpy(trf_frame->addr_src, my_addr, 6);
    memcpy(trf_frame->addr_dest, destination_addr, 6);
    trf_frame->duration = duration;

    // Now, fill the remainder of the payload with some
    // data (just to have something that is verificable on
    // the receiver end).
    int l = 0;
    int i;
    for (i = 0; i < 1000; i++)
    {
        trf_frame->payload[l++] = (char)((i % 42) + 48);
    }
    trf_frame->payload[l] = (char)0;

    // Set the packet length. It is the length of the payload
    // because it is a string, plus the size of the frame.
    //  Frame also includes extra pointer, which is offset by \0 at the end of string.
    this->packet.length = strlen((char *)trf_frame->payload) + sizeof(ieeeFrame);

    this->packet.length = data_length + sizeof(ieeeFrame);
}

TRAFFIC_GEN::TRAFFIC_GEN(SEND_PROTOCOL *protocol, uint8_t my_addr[6], uint8_t destination_addr[6], uint16_t duration, uint16_t message_length, char *message)
{

    this->running = false;
    this->send_protocol = protocol;

    ieeeFrame *trf_frame = (ieeeFrame *)this->packet.data;
    PACKET_TO_DATA(trf_frame);

    memcpy(trf_frame->addr_src, my_addr, 6);
    memcpy(trf_frame->addr_dest, destination_addr, 6);
    trf_frame->duration = duration;

    memcpy(trf_frame, message, message_length);

    this->packet.length = message_length + sizeof(ieeeFrame);
}

bool TRAFFIC_GEN::init()
{
    this->running = true;

    while (this->running)
    {
        switch (time_interval_mode)
        {
        case TRF_GEN_CONST:
            time_to_next = time_interval;
            break;
        case TRF_GEN_GAUSS:
            time_to_next = getTimeFromGauss();
            break;
        default:
            return false;
        }

        delay(time_to_next);

        this->send_protocol->send_data(this->packet);
    }

    return true;
}

float gauss_arr[6] = {0.66f, 0.10f, 0.2f};

uint16_t TRAFFIC_GEN::getTimeFromGauss()
{
    float pseudo_rand = (float)(millis() % 100) / 100; // Find better random
    uint16_t delay;

    if (pseudo_rand > gauss_arr[0])
        delay = (uint16_t)time_interval;
    else if (pseudo_rand > gauss_arr[1])
        delay = (uint16_t)(gauss_arr[0] * time_interval);
    else if (pseudo_rand > gauss_arr[2])
        delay = (uint16_t)(gauss_arr[1] * time_interval);
    else
        delay = (uint16_t)(gauss_arr[2] * time_interval);

    return delay;
}

void TRAFFIC_GEN::stop()
{
    this->running = false;
}

bool TRAFFIC_GEN::isRunning()
{
    return this->running;
}

bool TRAFFIC_GEN::setTime(uint8_t time_mode, uint16_t waiting_time)
{
    if (time_mode != TRF_GEN_CONST && time_mode != TRF_GEN_GAUSS)
        return false;

    if (this->running)
        return false;

    time_interval_mode = time_mode;
    time_interval = waiting_time;

    return true;
}

void TRAFFIC_GEN::setDestAddress(uint8_t addr[MAC_ADDRESS_SIZE])
{
    ieeeFrame *trf_frame = (ieeeFrame *)this->packet.data;
    memcpy(trf_frame->addr_dest, addr, MAC_ADDRESS_SIZE);
}

void TRAFFIC_GEN::setMessage(char *message, uint16_t message_length)
{
    ieeeFrame *trf_frame = (ieeeFrame *)this->packet.data;

    memcpy(trf_frame->payload, message, message_length);
    // trf_frame->duration = duration;

    this->packet.length = message_length + sizeof(ieeeFrame);
}