#include "Esp_Group.h"

/**
 * Exception logic handling extracted from
 * https://github.com/gammasoft71/Examples_FLTK/blob/master/src/Applications/Application_And_Exception/
 */

void Esp_Group::fltk_get_status(){
  status s;
  get_status(this->ip_address, &s);
}

void Esp_Group::fltk_set_time()
{
  const char *time_str = Time_input->value();
  if (time_str[0] != '\0')
  {
    if (!set_time(this->ip_address, (time_option) Time_type_choice->value(), (uint16_t)atoi(time_str)))
      throw std::invalid_argument{"Exception generated"};
  }
}

void Esp_Group::fltk_set_destination()
{
  // added conversion due to warnings, TODO check if is bad
  //Fl_Choice s = this.Backoff_choice;
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
  if (!set_backoff(this->ip_address, (backoff_option) Backoff_choice->value()))
    throw std::invalid_argument{"Exception generated"};
}