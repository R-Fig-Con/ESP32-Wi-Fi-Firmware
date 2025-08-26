#include "utils.h"
#include <stdexcept>
#include <FL/fl_message.H> 

/**
 * Exception logic handling extracted from 
 * https://github.com/gammasoft71/Examples_FLTK/blob/master/src/Applications/Application_And_Exception/
 */

Fl_Window *G_win = NULL;

Fl_Group* time_group = NULL;

Fl_Int_Input* Time_input = NULL;

Fl_Choice* Time_type_choice = NULL;
 
Fl_Choice* Backoff_choice = NULL;

Fl_Choice* Address_choice = NULL;

Fl_Input* Text_input = NULL;


static bool show_message(const char* title, const char* text) {
  struct Message_Params {
    Message_Params() = default;
    ~Message_Params() {
      fl_message_hotspot(hotspot);
      fl_message_icon()->box(box);
      fl_message_icon()->color(color);
      fl_message_icon()->label(label);
      fl_message_icon()->labelcolor(labelcolor);
      fl_message_icon()->labelfont(labelfont);
      fl_message_icon()->show();
      fl_message_title("");
    }
    
    int hotspot =  fl_message_hotspot();
    Fl_Boxtype box = fl_message_icon()->box();
    Fl_Color color = fl_message_icon()->color();
    Fl_Color labelcolor = fl_message_icon()->labelcolor();
    Fl_Font labelfont = fl_message_icon()->labelfont();
    const char* label = fl_message_icon()->label();
  } message_params;
  
  fl_message_hotspot(false);
  fl_message_icon()->box(FL_ROUND_UP_BOX);
  fl_message_icon()->color(fl_rgb_color(255, 0, 0));
  fl_message_icon()->label("X");
  fl_message_icon()->labelcolor(fl_rgb_color(255, 255, 255));
  fl_message_icon()->labelfont(FL_HELVETICA_BOLD);
  fl_message_title(title);
  return fl_choice("Unhandled exception occured in your application. If you click\n"
                   "Continue, the application will ignore this error and attempt to continue.\n"
                   "If you click Quit, the application will close immediately.\n"
                   "\n"
                   "%s", "&Quit", "&Continue", nullptr, text);
}

int event_dispatch(int event, Fl_Window* window) {
  try {
    return Fl::handle_(event, window);
  } catch (const std::exception& e) {
    if (!show_message("Esp exception", e.what())) exit(-1);
  }
  return 0;
}

void fltk_set_time(){
    const char* time_str = Time_input->value();
    if(time_str[0] != '\0'){
        if(!set_time((time_option) Time_type_choice->value(), (uint16_t) atoi(time_str))) 
            throw std::invalid_argument {"Exception generated"};
    }
}

void fltk_set_destination(){
  //added conversion due to warnings, TODO check if is bad
  switch (Address_choice->value()){
  case ADDRESS_FIRST_CHOICE:
    if(!set_destination((char*) ADDRESS_FIRST_VALUE)) 
      throw std::invalid_argument {"Exception generated"};
    break;
  case ADDRESS_SECOND_CHOICE:
    if(!set_destination((char*) ADDRESS_SECOND_VALUE)) 
      throw std::invalid_argument {"Exception generated"};
    break;
  case ADDRESS_THIRD_CHOICE:
    if(!set_destination((char*) ADDRESS_THIRD_VALUE)) 
      throw std::invalid_argument {"Exception generated"};
    break;
  default:
    break;
  }
}

void fltk_set_message(){
    char* read = (char*) Text_input->value();
    size_t size = strlen(read);

    if (size == 0){
      return;
    }
    
    if(!set_message(read, (uint16_t) size))
      throw std::invalid_argument {"Exception generated"};
    
}

void fltk_set_backoff(){
    if(!set_backoff((backoff_option) Backoff_choice->value())) 
        throw std::invalid_argument {"Exception generated"};
}

void fltk_get_status(status* mem);