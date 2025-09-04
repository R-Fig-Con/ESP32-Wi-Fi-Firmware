#define ESP_RESP_OK (char)0x0
#define ESP_RESP_ERROR (char)0x1

#define NUM_OPTIONS 5
#define STATUS_OPT_CODE 's'
#define MESSAGE_OPT_CODE 'm'
#define TIME_OPT_CODE 't'
#define DEST_OPT_CODE 'd'
#define BACKOFF_OPT_CODE 'b'

struct OPTION
{
    const char opt_code;
    void (*const handler)(WiFiClient *client, uint8_t *buffer, uint16_t len);
};

void wifi_handle_status(WiFiClient *client, uint8_t *buffer, uint16_t len);

void wifi_handle_message(WiFiClient *client, uint8_t *buffer, uint16_t len);

void wifi_handle_time(WiFiClient *client, uint8_t *buffer, uint16_t len);

void wifi_handle_destination(WiFiClient *client, uint8_t *buffer, uint16_t len);

void wifi_handle_backoff(WiFiClient *client, uint8_t *buffer, uint16_t len);

const struct OPTION options[NUM_OPTIONS] = {
    {STATUS_OPT_CODE, wifi_handle_status},
    {MESSAGE_OPT_CODE, wifi_handle_message},
    {TIME_OPT_CODE, wifi_handle_time},
    {DEST_OPT_CODE, wifi_handle_destination},
    {BACKOFF_OPT_CODE, wifi_handle_backoff}};