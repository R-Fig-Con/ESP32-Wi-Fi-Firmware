/**
 * Declaration of expected gui elements, fltk wrapper for error showing
 */

#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>

#include "./esp_communication/communication.h"

#define ADDRESS_FIRST_CHOICE 1
#define ADDRESS_SECOND_CHOICE 2
#define ADDRESS_THIRD_CHOICE 3
#define ADDRESS_FIRST_VALUE "0X12_34_56_78_9A_BC"
#define ADDRESS_SECOND_VALUE "0XFF_FF_FF_FF_FF_FF"
#define ADDRESS_THIRD_VALUE "0XAA_AA_AA_AA_AA_AA"

class Esp_Group: public Fl_Group
{
public:
    char *ip_address;
    /**
     * group containing  time inputs, the number and interval type
     */
    Fl_Group* Time_group;

    /**
     * Number time interval
     */
    Fl_Int_Input* Time_input;

    /**
     * Time interval type
     */
    Fl_Choice* Time_type_choice;

    /**
     * backoff type choice
     */
    Fl_Choice* Backoff_choice;

    /**
     * Mac destination address
     */
    Fl_Choice* Address_choice;

    /**
     * Message text set
     */
    Fl_Input* Text_input;

public:

    Esp_Group(char* address,int X, int Y, int W, int H);

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
};
