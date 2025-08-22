#include <arpa/inet.h> //uint16_t, unit_32t


#define RETURN_SUCCESS 0
#define RETURN_ESP_ERROR 1

#define MAC_ADDRESS_SIZE 6

#define MAX_MESSAGE_SIZE 1024
/**
 * Contains error message from esp in operation failure
 * 
 * Failing on connection start will not receive a message
 */
extern char communication_buffer[MAX_MESSAGE_SIZE];

typedef enum{
    GAUSSIAN = 0,
    LINEAR_TIME= 1,
} time_option;

typedef enum{
    LINEAR = 0,
    MILD = 1,
    NONE = 2,
} backoff_option;

typedef struct{
    char type;
    uint16_t time;
    uint16_t success_count;
    uint16_t failure_count;
    uint32_t retry_count;
    uint32_t running_time;
    unsigned char dest_mac[MAC_ADDRESS_SIZE];
    char* msg;
} status;

/**
 * connect to esp_32
 */
int connection_start();

/**
 * end communication
 */
void connection_end();

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
 * @param data message containing string
 * 
 * @param length the length of the data
*/
int set_message(char* data, uint16_t length);

/**
 * Set the backoff algorithm
 * 
 * @param algorithm the backoff algorithm
 */
int set_backoff(backoff_option option);

/**
 * Get status updatefrom the esp device
 * 
 * @param mem pointer to status. Function will update it with received
 * values. Unchanged and invalid if return is error indicator
 * status.message will have malloc? TODO CHECK
 */
int get_status(status* mem);