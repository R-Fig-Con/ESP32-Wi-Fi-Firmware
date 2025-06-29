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
        c = buffer[i];
        
        if(c>='0' && c<='9'){
            result.value *= 10;
            result.value += (uint16_t)(c - '0');
        }else{
            if(c!='\0' && c!='\n'){
                result.status = IS_NOT_NUMBER;
            }
            break;
        }
    }

    return result;
}

int is_valid_hex(char ch){
    if( ch >= 'A' && ch <= 'F') return 1;
    if( ch >= 'a' && ch <= 'f') return 1;
    if( ch >= '0' && ch <= '9') return 1;
    return 0;
}

char get_val_from_hex(char ch){
    if( ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if( ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if( ch >= '0' && ch <= '9') return ch - '0';
    return 0;
}

int get_mac_from_str(char *buffer, int buff_size, char *mac_addr, int addr_size){

    if( buff_size<=MAC_STR_LEN ) return MAC_IS_INVALID;
    if( buffer[MAC_STR_LEN] != '\n' && buffer[MAC_STR_LEN] != '\0'){
        printf("What...?");
        return MAC_IS_INVALID;
    }

    int buff_idx = 0, addr_idx = 0;
    
    for(; buff_idx<buff_size && buff_idx<MAC_STR_LEN; buff_idx++ ){
        if( buffer[buff_idx]=='\n' || buffer[buff_idx]=='\0' ) return MAC_IS_INVALID;
        if( (buff_idx+1)%3 == 0 ){
            if( buffer[buff_idx] != ':' ){
                printf("No : where it should be! %c", buffer[buff_idx]);
                return MAC_IS_INVALID;
            }
            continue;
        }
        if( !is_valid_hex(buffer[buff_idx]) ){
            printf("Not valid hex!");
            return MAC_IS_INVALID;
        }
    }
    buff_idx = 0;
    

    while( addr_idx<addr_size ){
        char part1 = get_val_from_hex(buffer[buff_idx++]); //buff_idx incs after call
        char part2 = get_val_from_hex(buffer[buff_idx++]); //buff_idx incs after call
        mac_addr[addr_idx++] = ((part1<<4) + part2) & 0xFF;
        buff_idx++; //skip ':'

        //printf("hex = %02X | part1 = %02X | part2 = %02X\n", mac_addr[addr_idx-1], part1, part2); //DEBUG
    }

    return MAC_IS_VALID;
}