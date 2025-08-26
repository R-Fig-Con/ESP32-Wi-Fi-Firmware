#include "communication.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(){

    assert(connection_start());

    printf("Connection start done");

    assert(set_time(LINEAR_TIME, 200));

    printf("Set time done");

    assert(set_destination("0XAA_AA_AA_AA_AA_AA"));

    char msg[] = "new message";

    assert(set_message(msg, strlen(msg)));

    assert(set_backoff(NONE));

    connection_end();

}
