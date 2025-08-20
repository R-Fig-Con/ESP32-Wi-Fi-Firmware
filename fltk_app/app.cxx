
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>

#include <FL/Fl_Return_Button.H>

#include "./esp_communication/communication.h"


#define BACKOFF_MILD_CHOICE 1
#define BACKOFF_NONE_CHOICE 2
#define BACKOFF_LINEAR_CHOICE 3

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

void backoff_choice_callback(Fl_Widget *w, void*){
    w->argument();

}


void call(Fl_Widget*, void*){
    printf("Time in value: %s\n", Time_input->value());

    printf("Message input value: %s\n", Text_input->value());

    printf("Time interval type: %d\n", Time_type_choice->value());

    printf("Backoff type: %d\n", Backoff_choice->value());

    printf("Address type value: %d\n", Address_choice->value());

    time_group->hide();
}

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

    G_win = new Fl_Window(1000, 510, "App");

    char buffer[127];

    //test button
    Fl_Return_Button* b = new Fl_Return_Button(20, 10, 160, 35, buffer); b->callback(call);

    int y_measure = Y_START;

    time_group = new Fl_Group(X_START, y_measure, X_SIZE + 100, Y_SIZE + 100, "BOX DARK RED");
    //time_group->begin();// alredy done by default
    time_group->color(FL_DARK_RED); time_group->redraw();
    //time_group->hide();

    y_measure += 10;

    Time_type_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Interval type: ");
    Time_type_choice->add("Gaussian", 0, backoff_choice_callback, (void*) 0);
    Time_type_choice->add("Linear", 0, backoff_choice_callback, (void*) 1);

    y_measure += Y_SIZE + Y_SPACING; 

    //time_group->add(Time_type_choice);

    Time_input = new Fl_Int_Input(X_START, y_measure, X_SIZE, Y_SIZE, "Time input");
    Time_input->color(FL_DARK_RED); //Time_input->value(20); //Time_input->handle();
    y_measure += Y_SIZE + Y_SPACING;

    //time_group->add(Time_input);
    time_group->end();

    Backoff_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Backoff: ");
    Backoff_choice->add("Mild", 0, backoff_choice_callback, (void*) BACKOFF_MILD_CHOICE);
    Backoff_choice->add("No backoff", 0, backoff_choice_callback, (void*) BACKOFF_NONE_CHOICE);
    Backoff_choice->add("Linear backoff", 0, backoff_choice_callback, (void*) BACKOFF_LINEAR_CHOICE);
    y_measure += Y_SIZE + Y_SPACING;

    
    Address_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Address: ");
    /**
     * As a choice app would show all found esps mac addresses
     */
    Address_choice->add("0X12_34_56_78_9A_BC", 0, NULL, (void*) BACKOFF_MILD_CHOICE);
    Address_choice->add("0XFF_FF_FF_FF_FF_FF", 0, NULL, (void*) BACKOFF_NONE_CHOICE);
    Address_choice->add("0XAA_AA_AA_AA_AA_AA", 0, NULL, (void*) BACKOFF_LINEAR_CHOICE);
    y_measure += Y_SIZE + Y_SPACING; Address_choice->argument();
    

    Text_input = new Fl_Input(X_START, y_measure, X_SIZE, Y_SIZE, "Message input");
    y_measure += Y_SIZE + Y_SPACING;

    //G_win->end();
    G_win->show();
    return(Fl::run());
}
