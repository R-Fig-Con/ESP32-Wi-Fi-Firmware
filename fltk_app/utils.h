/**
 * Declaration of expected gui elements, fltk wrapper for error showing
 */

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>

#include "./esp_communication/communication.h"


#define ADDRESS_FIRST_CHOICE 1
#define ADDRESS_SECOND_CHOICE 2
#define ADDRESS_THIRD_CHOICE 3
#define ADDRESS_FIRST_VALUE  "0X12_34_56_78_9A_BC"
#define ADDRESS_SECOND_VALUE "0XFF_FF_FF_FF_FF_FF"
#define ADDRESS_THIRD_VALUE "0XAA_AA_AA_AA_AA_AA"

// Globals
extern Fl_Window *G_win;

/**
 * group containing  time inputs, the number and interval type
 */
extern Fl_Group* time_group;

/**
 * Number time interval
 */
extern Fl_Int_Input* Time_input;

/**
 * Time interval type
 */
extern Fl_Choice* Time_type_choice;
 
/**
 * backoff type choice
 */
extern Fl_Choice* Backoff_choice;

/**
 * Mac destination address
 */
extern Fl_Choice* Address_choice;

/**
 * Message text set
 */
extern Fl_Input* Text_input;


/**
 * function necessary to handle errors sent from other functions 
 * declared in this file
 */
int event_dispatch(int event, Fl_Window* window);


/**
 * Set time interval 
 */
void fltk_set_time();

/**
 * Set mac address
 */
void fltk_set_destination();

/**
 * Send data to Esp. Call after communication_start and
 * before communication_end
*/
void fltk_set_message();

/**
 * Set the backoff algorithm
 */
void fltk_set_backoff();

/**
 * Get status updatefrom the esp device
 */
void fltk_get_status();