#include <arpa/inet.h> //for uint16_t

#define ESP_PORT 5000
#define ESP_IP "10.0.0.1"

//not all may be necessary
#define RETURN_SUCCESS 0
#define RETURN_TERMINATE 1
#define RETURN_INPUT_ERROR 2
#define RETURN_ESP_ERROR 3
#define RETURN_APP_ERROR 4

#define TERMINATE_OPT_CODE 'x'
#define STATUS_OPT_CODE 's'
#define MESSAGE_OPT_CODE 'm'
#define TIME_OPT_CODE 't'
#define DEST_OPT_CODE 'd'
#define BACKOFF_PROTOCOL_OPT_CODE 'b'

#define MAC_ADDRESS_SIZE 6

typedef enum{
    GAUSSIAN = 0,
    LINEAR = 1,
} time_option;

/**
 * connect to esp_32
 */
int connection_start();

/**
 * end communication
 */
int connection_end();

/**
 * Set time interval 
 * 
 * @param time_type tipe of interval chosen
 * 
 * @param number time interval in millisseconds
 */
int set_time(time_option time_type, uint16_t number);

/**
 * Set mac address 
 * 
 * @param address the mac address in hexadecimal string
 */
int set_destination(char address[MAC_ADDRESS_SIZE]);

/**
 * Send data to Esp. Call after communication_start and
 * before communication_end
 * 
 * @param option  choose the kind of data to send
 * 
 * @param data char string containing data
 * 
 * @param length the length of the data
*/
static int set_message(char option, char* data, uint16_t length);

/**
 * Set the backoff algorithm
 * 
 * @param algorithm the backoff algorithm
 */
static int set_backoff(char algorithm);