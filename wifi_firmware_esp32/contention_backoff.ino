uint8_t CONTENTION_BACKOFF::getBackoff(){
    //https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/random.html#_CPPv410esp_randomv
    //https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand
    //minimum as 0
    return esp_random() / (RAND_MAX / (contentionWindow + 1) + 1);
}