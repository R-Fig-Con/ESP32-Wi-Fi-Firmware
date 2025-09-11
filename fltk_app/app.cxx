#include <stdlib.h>  // memory allocation
#include <pthread.h> // threads

#include <FL/Fl_Sys_Menu_Bar.H> //top menu bar
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>

#include "Esp_Group.h"
#include "instances_search/instances_search.h"
#include "exception_handling.h"

/*list can probably be substituted by Fl_Group::array() */
typedef struct node
{
  const char *address;
  Esp_Group *grp;
  struct node *next;
} group_list;

static group_list *head = NULL;

static void add_node(const char *key, Esp_Group *value)
{
  group_list *current = head;
  if (head == NULL)
  {
    current = (group_list *)malloc(sizeof(group_list));
    current->address = key;
    current->grp = value;
    head = current;
    return;
  }

  group_list *next = head->next;

  while (1)
  {
    if (next == NULL)
    {
      next = (group_list *)malloc(sizeof(group_list));
      next->address = key;
      next->grp = value;
      current->next = next;
      return;
    }

    current = current->next;
    next = next->next;
  }
}

static Esp_Group *get_group(const char *key)
{
  group_list *current = head;

  while (1)
  {
    if (!strcmp(current->address, key))
    {
      return current->grp;
    }

    current = current->next;
  }
}

static void remove_node(const char *key)
{
  group_list *prev = head;

  if (!strcmp(prev->address, key))
  {
    head = head->next;
    free(prev);
    return;
  }

  group_list *current = head->next;

  while (1)
  {
    if (!strcmp(current->address, key))
    {
      prev->next = current->next;
      free(current);
      return;
    }

    current = current->next;
    prev = prev->next;
  }
}

/**
 * Returns which group is currently being shown
 */
static Esp_Group *get_visible()
{
  group_list *current = head;

  while (1)
  {
    if (current == NULL)
    {
      return NULL;
    }

    if (current->grp->visible())
    {
      return current->grp;
    }

    current = current->next;
  }
}
void print(Fl_Widget *, void *)
{

  Esp_Group* visible = get_visible();

  if (visible == NULL)
  {
    return;
  }
  

  printf("Time in value: %s\n", visible->Time_input->value());

  printf("Message input value: %s\n", visible->Text_input->value());

  printf("Time interval type: %d\n", visible->Time_type_choice->value());

  printf("Backoff type: %d\n", visible->Backoff_choice->value());

  printf("Address type value: %d\n", visible->Address_choice->value());

  // check if empty string is a null value
  if (visible->Text_input->value() == NULL)
  {
    printf("Message string pointer is null\n");
  }
  else
  {
    printf("Message string pointer is NOT null\n");
  }

}

/**
 * Element that can cycle around every gui input screen
 * of every connected esp from instances_search
 *
 * When further level of knowledge of the fltk library is
 * achieved, attempt to create class to encompass properties
 * in a more readable manner
 */
Fl_Tabs *esp_interfaces_group;

void instance_found_action(char address[IP_ADDRESS_MAX_SIZE])
{
  if (!connection_start(address))
  {
    return;
  }

  status s; // get_status(address, &s); // todo display starting info

  Fl::lock();
  esp_interfaces_group->begin();
  Esp_Group *g = new Esp_Group(
      address,
      s,
      esp_interfaces_group->x(),
      esp_interfaces_group->y() + 25, // following tabs-simple.cxx example
      esp_interfaces_group->w(),
      esp_interfaces_group->h());
  add_node(address, g);
  esp_interfaces_group->end();
  esp_interfaces_group->redraw(); // seems necessary, else it may not draw all
  printf("instance found end\n\n");
  Fl::unlock();
}

// lots of questions about memory managment with fltk, a lot of this needs
// to be checked
void instance_left_action(char address[IP_ADDRESS_MAX_SIZE])
{

  Fl::lock();
  Esp_Group *g = get_group(address);
  delete g;
  remove_node(address); // test with Fl::delete_widget() too
  printf("instance left end\n");
  Fl::unlock();
}

Fl_Menu_Item menutable[] = {
    {"&Test menu", 0, 0, 0, FL_SUBMENU},

    {"print values of shown", 0, print},
    {"Undo", 0, 0},
    {0},

    {"foo",0,0,0,FL_MENU_INACTIVE},

    {0},};

void *instance_search_thread_function(void *)
{
  start_instance_search();

  return NULL;
}

/**
 * Items belonging on box but not on area are drawn, but seem impossible to interact
 */
int main() // valgrind --leak-check=full -s --show-leak-kinds=all ./bin/app
{
  Fl::scheme("gtk+");
  Fl_Window *G_win = new Fl_Window(1000, 750, "App"); //; G_win->set_modal();

  Fl_Menu_Bar menubar (0,0,1000,30); menubar.menu(menutable);

  esp_interfaces_group = new Fl_Tabs(50, 50, 750, 600);

  on_instance_found_event(instance_found_action);
  on_instance_left_event(instance_left_action);

  pthread_t search_thread;
  Fl::lock();
  pthread_create(&search_thread, NULL, instance_search_thread_function, NULL);

  G_win->resizable(G_win);
  G_win->show();
  Fl::event_dispatch(exception_handler);
  int ret = Fl::run();

  end_instance_search();

  delete esp_interfaces_group;
  delete G_win;

  return ret;
}
