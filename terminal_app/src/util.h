char get_char_and_flush();

#define IS_NUMBER 0
#define IS_NOT_NUMBER 1

struct NUMBER{
    char status;
    int value;
};

struct NUMBER get_number_from_str(const char *buffer, int size);

void fit_number_w_left_padding(char *buffer, unsigned int bits, unsigned int number);