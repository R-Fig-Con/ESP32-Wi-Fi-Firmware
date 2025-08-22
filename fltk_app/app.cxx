#include <stdlib.h> // atoi

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>


#include <FL/Fl_Sys_Menu_Bar.H> //top menu bar
#include <FL/fl_ask.H> // pop up window
#include <FL/Fl_Return_Button.H>

#include "./esp_communication/communication.h"


#define BACKOFF_MILD_CHOICE MILD
#define BACKOFF_NONE_CHOICE NONE
#define BACKOFF_LINEAR_CHOICE LINEAR

#define LINEAR_TIME_INTERVAL LINEAR_TIME
#define GAUSSIAN_TIME_INTERVAL GAUSSIAN

#define ADDRESS_FIRST_CHOICE 1
#define ADDRESS_SECOND_CHOICE 2
#define ADDRESS_THIRD_CHOICE 3
#define ADDRESS_FIRST_VALUE  "0X12_34_56_78_9A_BC"
#define ADDRESS_SECOND_VALUE "0XFF_FF_FF_FF_FF_FF"
#define ADDRESS_THIRD_VALUE "0XAA_AA_AA_AA_AA_AA"

#define X_START 250
#define Y_START 50

#define X_SIZE 150
#define Y_SIZE 25

#define Y_SPACING 60


// Globals
Fl_Window *G_win = NULL;

/**
 * group containing  time inputs, the number and interval type
 */
Fl_Group* time_group = NULL;

/**
 * Number time interval
 */
Fl_Int_Input* Time_input = NULL;

/**
 * Time interval type
 */
Fl_Choice* Time_type_choice = NULL;
 
/**
 * backoff type choice
 */
Fl_Choice* Backoff_choice = NULL;

/**
 * Mac destination address
 */
Fl_Choice* Address_choice = NULL;

/**
 * Message text set
 */
Fl_Input* Text_input = NULL;

/**
 * change to red as to mark change to send?
 */
void choice_callback(Fl_Widget *w, void*){
    w->color(FL_RED);
}

void set_status_timer_callback(){

}


void call(Fl_Widget*, void*){
    printf("Time in value: %s\n", Time_input->value());

    printf("Message input value: %s\n", Text_input->value());

    printf("Time interval type: %d\n", Time_type_choice->value());

    printf("Backoff type: %d\n", Backoff_choice->value());

    printf("Address type value: %d\n", Address_choice->value());

    //check if empty string is a null value
    if(Text_input->value() == NULL){
      printf("Message string pointer is null\n");
    }else{
      printf("Message string pointer is NOT null\n");
    }

    //testing to see what disappears
    //time_group->hide();
}

/**
 * This callback will send all data, even the ones user has not changed
 * Only exception is empty text boxes
 */
void send_all_data_sets(Fl_Widget*, void*){

    char* msg = (char*) Text_input->value();
  if(msg[0] != '\0'){
    set_message(msg, (uint16_t) strlen(msg));
  }

  //added conversion due to warnings, TODO check if is bad
  switch (Address_choice->value()){
  case ADDRESS_FIRST_CHOICE:
    set_destination((char*) ADDRESS_FIRST_VALUE);
    break;
  case ADDRESS_SECOND_CHOICE:
    set_destination((char*) ADDRESS_SECOND_VALUE);
    break;
  case ADDRESS_THIRD_CHOICE:
    set_destination((char*) ADDRESS_THIRD_VALUE);
    break;
  default:
    break;
  }

  set_backoff((backoff_option) Backoff_choice->value());
  
  const char* time_str = Time_input->value();
  if(time_str[0] != '\0'){
    set_time((time_option) Time_type_choice->value(), (uint16_t) atoi(time_str));
  }
  
}

Fl_Menu_Item menutable[] = {
  {"&Status timer",0,0,0,FL_SUBMENU},
    {"set interval", 0,  0},
    {"Undo", 0, 0},
    {0},
  {0}
};

/*
void rename_button(Fl_Widget *o, void *v) {
  int what = fl_int(v);
  int ret = 0;
  std::string input;
  if (what == 0) {
    fl_message_icon_label("§");
    input = fl_input_str(ret, 0, "Input (no size limit, use ctrl/j for newline):", o->label());
  } else {
    fl_message_icon_label("€");
    input = fl_password_str(ret, 20, "Enter password (max. 20 characters):", o->label());
  }
  if (ret == 0) {
    o->copy_label(input.c_str());
    o->redraw();
  }
}
*/

/**
 * personal note about group:
 * 
 * if group is created on main all objects afterwards will belong to the group by default
 * 
 * calling end() method  no object will belong to it, unless it is added through add()
 * 
 * Items belonging on box but not on area are drawn, but seem impossible to interact
 * 
 */
int main() {

    G_win = new Fl_Window(1000, 510, "App"); //; G_win->set_modal();

    //Fl_Menu_Bar menubar (0,0,1000,30); menubar.menu(menutable);

    char buff[127] = "Print values test";

    //test button
    Fl_Return_Button* b = new Fl_Return_Button(600, 100, 160, 35, buff); b->callback(call);

    Fl_Button* send = new Fl_Button(600, 300, 160, 35, "Send all data");
    send->callback(send_all_data_sets);

    int y_measure = Y_START;

    time_group = new Fl_Group(X_START, y_measure, X_SIZE + 100, Y_SIZE + 100, "BOX DARK RED");
    //time_group->begin();// alredy done by default
    time_group->color(FL_DARK_RED); time_group->redraw();
    //time_group->hide();

    y_measure += 10;

    Time_type_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Interval type: ");
    Time_type_choice->add("Gaussian", 0, NULL, (void*) LINEAR_TIME_INTERVAL);
    Time_type_choice->add("Linear", 0, NULL, (void*) GAUSSIAN_TIME_INTERVAL);
    Time_type_choice->value(0);
    y_measure += Y_SIZE + Y_SPACING; 

    Time_input = new Fl_Int_Input(X_START, y_measure, X_SIZE, Y_SIZE, "Time input");
    //Time_input->color(FL_DARK_RED);
    y_measure += Y_SIZE + Y_SPACING;

    time_group->end(); // needed

    Backoff_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Backoff: ");
    Backoff_choice->add("Mild", 0, NULL, (void*) BACKOFF_MILD_CHOICE);
    Backoff_choice->add("No backoff", 0, NULL, (void*) BACKOFF_NONE_CHOICE);
    Backoff_choice->add("Linear backoff", 0, NULL, (void*) BACKOFF_LINEAR_CHOICE);
    Backoff_choice->value(0);
    y_measure += Y_SIZE + Y_SPACING;

    
    Address_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Address: ");
    /**
     * As a choice app would show all found esps mac addresses
     */
    Address_choice->add("0X12_34_56_78_9A_BC", 0, NULL, (void*) BACKOFF_MILD_CHOICE);
    Address_choice->add("0XFF_FF_FF_FF_FF_FF", 0, NULL, (void*) BACKOFF_NONE_CHOICE);
    Address_choice->add("0XAA_AA_AA_AA_AA_AA", 0, NULL, (void*) BACKOFF_LINEAR_CHOICE);
    Address_choice->value(0);
    y_measure += Y_SIZE + Y_SPACING;
    

    Text_input = new Fl_Input(X_START, y_measure, X_SIZE, Y_SIZE, "Message input");
    y_measure += Y_SIZE + Y_SPACING;

    //G_win->end();
    G_win->show();
    return(Fl::run());
}
