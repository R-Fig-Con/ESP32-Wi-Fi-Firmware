#include <stdio.h>
#include "util.h"

char get_char_and_flush(){
    char flush;
    char input_code = getchar();
    while ((flush = getchar()) != '\n'); // Clear input buffer
    return input_code;
}

struct NUMBER get_number_from_str(const char *buffer, int size){

    struct NUMBER result = { IS_NUMBER, 0 };

    char c;

    for(int i = 0; i<size; i++){
        c = buffer[0];
        
        if(c>='0' && c<='9'){
            result.value *= 10;
            result.value += (int)(c - '0');
        }else{
            if(c!='\0' && c!='\n'){
                result.status = IS_NOT_NUMBER;
            }
            break;
        }
    }

    return result;
}

void fit_number_w_left_padding(char *buffer, unsigned int bits, unsigned int number){
    if(bits>31){
        buffer[0] = '\0';
        printf("Error: too many bits.");
        return;
    }

    unsigned int max_val = (1 << bits) - 1;
    if(number >= max_val){
        number = number & max_val; 
    }

    // How many bytes needed
    int num_bytes = (bits + 7) / 8;
    //memset(buffer, 0, num_bytes);

    for (int i = 0; i < num_bytes; ++i) {
        buffer[num_bytes - 1 - i] = (number >> (8 * i)) & 0xFF;
    }
}