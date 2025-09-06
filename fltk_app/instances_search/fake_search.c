/**
 * Purpose of this fake is to test if actions given
 * by on_instance_found_event and on_instance_left_event
 * are accurate
 * 
 * Starting by checking freeing memory on gui
 */

#include "instances_search.h"

#include <unistd.h>


static instance_search_event* found_action;

static instance_search_event* left_action;

void on_instance_found_event(instance_search_event action){
    found_action = action;
}

void on_instance_left_event(instance_search_event action){
    left_action = action;
}

volatile int end = 0;

char fake_test_instance[] = "fake result";
void start_instance_search(){

    while (!end)
    {
        sleep(5);

        found_action(fake_test_instance);

        sleep(10);
        left_action(fake_test_instance);
    }
    

}


void end_instance_search(){
    end = 1;
}
