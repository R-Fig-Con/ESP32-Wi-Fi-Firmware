#include "Esp_Group.h"

#include <stdexcept>

#define X_SIZE 150
#define Y_SIZE 25

#define Y_SPACING 60

/**
 * Exception logic handling extracted from
 * https://github.com/gammasoft71/Examples_FLTK/blob/master/src/Applications/Application_And_Exception/
 */

void Esp_Group::fltk_get_status()
{
  status s;
  get_status(this->ip_address, &s);
}

void Esp_Group::fltk_set_time()
{
  const char *time_str = Time_input->value();
  if (time_str[0] != '\0')
  {
    if (!set_time(this->ip_address, (time_option)Time_type_choice->value(), (uint16_t)atoi(time_str)))
      throw std::invalid_argument{"Exception generated"};
  }
}

void Esp_Group::fltk_set_destination()
{
  // added conversion due to warnings, TODO check if is bad
  // Fl_Choice s = this.Backoff_choice;
  switch (Address_choice->value())
  {
  case ADDRESS_FIRST_CHOICE:
    if (!set_destination(this->ip_address, (char *)ADDRESS_FIRST_VALUE))
      throw std::invalid_argument{"Exception generated"};
    break;
  case ADDRESS_SECOND_CHOICE:
    if (!set_destination(this->ip_address, (char *)ADDRESS_SECOND_VALUE))
      throw std::invalid_argument{"Exception generated"};
    break;
  case ADDRESS_THIRD_CHOICE:
    if (!set_destination(this->ip_address, (char *)ADDRESS_THIRD_VALUE))
      throw std::invalid_argument{"Exception generated"};
    break;
  default:
    break;
  }
}

void Esp_Group::fltk_set_message()
{
  char *read = (char *)Text_input->value();
  size_t size = strlen(read);

  if (size == 0)
  {
    return;
  }

  if (!set_message(this->ip_address, read, (uint16_t)size))
    throw std::invalid_argument{"Exception generated"};
}

void Esp_Group::fltk_set_backoff()
{
  if (!set_backoff(this->ip_address, (backoff_option)Backoff_choice->value()))
    throw std::invalid_argument{"Exception generated"};
}

/*
void Esp_Group::input_callback(Fl_Widget *widget, void *)
{
  Fl_Input *input = (Fl_Input *)widget;

  if (input == this->Time_input)
  {
    if (atoi(Time_input->value()) == this->setup_values.time)
    {
      widget->color(FL_DARK_RED);
    } else{
      widget->color(FL_WHITE);
    }
  }

}

*/

Esp_Group::Esp_Group(char *address, status s, int X, int Y, int W, int H) : Fl_Group(X, Y, W, H, "Text")
{
  this->setup_values = s;
  this->ip_address = address;

  int current_y = Y + Y_SPACING;

  int x_start = X + 150;
  this->Time_group = new Fl_Group(x_start, current_y, X_SIZE + 100, Y_SIZE + 100, "Time group");
  Time_group->color(FL_DARK_RED);
  Time_group->redraw();
  // time_group->hide();

  current_y += 10;

  Time_type_choice = new Fl_Choice(x_start, current_y, X_SIZE, Y_SIZE, "Interval type: ");
  Time_type_choice->add("Gaussian", 0, NULL, (void *)'g');
  Time_type_choice->add("Linear", 0, NULL, (void *)'l');
  Time_type_choice->value(0);

  current_y += Y_SIZE + Y_SPACING;

  Time_input = new Fl_Int_Input(x_start, current_y, X_SIZE, Y_SIZE, "Time input");

  current_y += Y_SIZE + Y_SPACING;

  Time_group->end(); // needed

  Backoff_choice = new Fl_Choice(x_start, current_y, X_SIZE, Y_SIZE, "Backoff: ");
  Backoff_choice->add("Linear backoff", 0, NULL, (void *)LINEAR);
  Backoff_choice->add("Mild", 0, NULL, (void *)MILD);
  Backoff_choice->add("No backoff", 0, NULL, (void *)NONE);
  Backoff_choice->value(0);
  current_y += Y_SIZE + Y_SPACING;

  Address_choice = new Fl_Choice(x_start, current_y, X_SIZE, Y_SIZE, "Address: ");
  /**
   * As a choice app would show all found esps mac addresses
   */
  Address_choice->add("0X12_34_56_78_9A_BC", 0, NULL, NULL);
  Address_choice->add("0XFF_FF_FF_FF_FF_FF", 0, NULL, NULL);
  Address_choice->add("0XAA_AA_AA_AA_AA_AA", 0, NULL, NULL);
  Address_choice->value(0);
  current_y += Y_SIZE + Y_SPACING;

  Text_input = new Fl_Input(x_start, current_y, X_SIZE, Y_SIZE, "Message input");

  Text_input->callback();
}