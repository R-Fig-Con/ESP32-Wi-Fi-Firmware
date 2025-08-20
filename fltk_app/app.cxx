
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>

#define BACKOFF_MILD_CHOICE 1
#define BACKOFF_NONE_CHOICE 2
#define BACKOFF_LINEAR_CHOICE 3

#define X_START 250
#define Y_START 50

#define X_SIZE 150
#define Y_SIZE 25

#define Y_SPACING 20


// Globals
Fl_Window *G_win = NULL;

void backoff_choice_callback(Fl_Widget *w, void*){

}


// main
int main() {

    G_win = new Fl_Window(1000, 510, "App");
    //G_win->begin();

    int y_measure = Y_START;

    Fl_Group* time_group = new Fl_Group(X_START, y_measure, X_SIZE + 100, Y_SIZE + 100, "BOX DARK RED");
    time_group->color(FL_DARK_RED); time_group->redraw(); 
    //time_group->hide();

    y_measure += 10;

    Fl_Choice* Time_type_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Interval type: ");
    Time_type_choice->add("Gaussian", 0, backoff_choice_callback, (void*) 0);
    Time_type_choice->add("Linear", 0, backoff_choice_callback, (void*) 1);

    y_measure += Y_SIZE + Y_SPACING;

    time_group->add(Time_type_choice);

    Fl_Int_Input* Time_input = new Fl_Int_Input(X_START, y_measure, X_SIZE, Y_SIZE, "Time input");
    Time_input->color(FL_DARK_RED);
    y_measure += Y_SIZE + Y_SPACING;

    time_group->add(Time_input);

    Fl_Choice* Backoff_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Backoff: ");
    Backoff_choice->add("Mild", 0, backoff_choice_callback, (void*) BACKOFF_MILD_CHOICE);
    Backoff_choice->add("No backoff", 0, backoff_choice_callback, (void*) BACKOFF_NONE_CHOICE);
    Backoff_choice->add("Linear backoff", 0, backoff_choice_callback, (void*) BACKOFF_LINEAR_CHOICE);
    y_measure += Y_SIZE + Y_SPACING;

    /**
     * As a choice app would show all found esps mac addresses
     */
    Fl_Choice* Address_choice = new Fl_Choice(X_START, y_measure, X_SIZE, Y_SIZE, "Address: ");
    Address_choice->add("0X12_34_56_78_9A_BC", 0, NULL, (void*) BACKOFF_MILD_CHOICE);
    Address_choice->add("0XFF_FF_FF_FF_FF_FF", 0, NULL, (void*) BACKOFF_NONE_CHOICE);
    Address_choice->add("0XAA_AA_AA_AA_AA_AA", 0, NULL, (void*) BACKOFF_LINEAR_CHOICE);
    y_measure += Y_SIZE + Y_SPACING;

    /**
     * Message text set
     */
    Fl_Input* Text_input = new Fl_Input(X_START, y_measure, X_SIZE, Y_SIZE, "Message input");
    y_measure += Y_SIZE + Y_SPACING;

    //G_win->end();
    G_win->show();
    return(Fl::run());
}
