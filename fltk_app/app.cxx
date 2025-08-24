#include <stdlib.h> // atoi
#include <stdexcept> //exceptions

#include <FL/Fl_Sys_Menu_Bar.H> //top menu bar
#include <FL/Fl_Button.H>

//#include <FL/Fl_Anim_GIF_Image.H>

#include "utils.h"


#define BACKOFF_MILD_CHOICE MILD
#define BACKOFF_NONE_CHOICE NONE
#define BACKOFF_LINEAR_CHOICE LINEAR

#define LINEAR_TIME_INTERVAL LINEAR_TIME
#define GAUSSIAN_TIME_INTERVAL GAUSSIAN

#define X_START 250
#define Y_START 50

#define X_SIZE 150
#define Y_SIZE 25

#define Y_SPACING 60

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

  fltk_set_message();
  
  fltk_set_destination();
  
  fltk_set_backoff();

  fltk_set_time();
  
}

Fl_Menu_Item menutable[] = {
  {"&Status timer",0,0,0,FL_SUBMENU},
    {"set interval", 0,  0},
    {"Undo", 0, 0},
    {0},
  {0}
};


/**
 * personal note about group:
 * 
 * if group is created on main all objects afterwards will belong to the group by default
 * 
 * calling end() method  no object will belong to it, unless it is added through add()
 * 
 * Items belonging on box but not on area are drawn, but seem impossible to interact
 */
int main() {

  if(!connection_start()){
    printf("Connection start failed");
    return 0;
  }

  G_win = new Fl_Window(1000, 510, "App"); //; G_win->set_modal();

  //Fl_Menu_Bar menubar (0,0,1000,30); menubar.menu(menutable);

  Fl_Button* b = new Fl_Button(600, 100, 160, 35, "Print values test"); b->callback(call);

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
  Fl::event_dispatch(event_dispatch);
  int ret = Fl::run();

  printf("Closing connection\n");
  connection_end();

  return ret;
}
