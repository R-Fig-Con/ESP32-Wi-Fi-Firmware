#include <stdint.h>
char get_char_and_flush();

#define IS_NUMBER 0
#define IS_NOT_NUMBER 1

struct NUMBER{
    char status;
    uint16_t value;
};

struct NUMBER get_number_from_str(const char *buffer, int size);

//void fit_number_w_left_padding(char *buffer, unsigned int bits, unsigned int number);

#define MAC_IS_VALID 0
#define MAC_IS_INVALID 1

#define MAC_STR_LEN 17 //(6*2+5)

int get_mac_from_str(char *buffer, int buff_size, char *mac_addr, int arr_size);