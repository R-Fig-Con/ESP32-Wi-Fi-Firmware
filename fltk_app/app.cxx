#include <stdlib.h>  // atoi
#include <pthread.h> // threads

#include <FL/Fl_Sys_Menu_Bar.H> //top menu bar
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>

#include "Esp_Group.h"
#include "instances_search/instances_search.h"
#include "exception_handling.h"

#define X_SIZE 150
#define Y_SIZE 25

#define Y_SPACING 60

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
        current = (group_list*) malloc(sizeof(group_list));
        current->address = key;
        current->grp = value;
        head = current;
        return;
    }
    
    group_list* next = head->next;

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
    if (strcmp(current->address, key))
    {
      return current->grp;
    }

    current = current->next;
  }
}

static void remove_node(const char *key)
{
  group_list *prev = head;

  group_list *current = head->next;

  while (1)
  {
    if (strcmp(current->address, key))
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
 * Element that can cycle around every gui input screen
 * of every connected esp from instances_search
 *
 * When further level of knowledge of the fltk library is
 * achieved, attempt to create class to encompass properties
 * in a more readable manner
 */
Fl_Tabs *esp_interfaces_group;

Esp_Group::Esp_Group(char *address, int X, int Y, int W, int H) : Fl_Group(X, Y, W, H)
{

  this->ip_address = address;

  int current_y = Y + Y_SPACING;

  int x_start = X;
  this->Time_group = new Fl_Group(x_start, current_y, X_SIZE + 100, Y_SIZE + 100, "BOX DARK RED");
  // time_group->begin();// alredy done by default
  Time_group->color(FL_DARK_RED);
  Time_group->redraw();
  // time_group->hide();

  current_y += 10;

  Time_type_choice = new Fl_Choice(x_start, current_y, X_SIZE, Y_SIZE, "Interval type: ");
  Time_type_choice->add("Gaussian", 0, NULL, (void *)LINEAR_TIME);
  Time_type_choice->add("Linear", 0, NULL, (void *)GAUSSIAN);
  Time_type_choice->value(0);
  current_y += Y_SIZE + Y_SPACING;

  Time_input = new Fl_Int_Input(x_start, current_y, X_SIZE, Y_SIZE, "Time input");
  // Time_input->color(FL_DARK_RED);
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
}

void instance_found_action(char address[IP_ADDRESS_MAX_SIZE])
{
  connection_start(address);

  printf("Connected\n");

  status s;
  get_status(address, &s); // todo display starting info

  printf("REceived status\n");

  Fl::lock();
  esp_interfaces_group->begin();
  Esp_Group *g = new Esp_Group(
      address,
      esp_interfaces_group->x(),
      esp_interfaces_group->y(),
      esp_interfaces_group->w(),
      esp_interfaces_group->h());
  add_node(address, g);
  esp_interfaces_group->end();
  Fl::unlock();
}

// lots of questions about memory managment with fltk, a lot of this needs
// to be checked
void instance_left_action(char address[IP_ADDRESS_MAX_SIZE]){

  Fl::lock();
  Esp_Group* g = get_group(address);
  delete g;
  remove_node(address); // test with Fl::delete_widget() too
  Fl::unlock();
}

/**
 * This callback will send all data, even the ones user has not changed
 * Only exception is empty text boxes
 */
/*
void send_all_data_sets(Fl_Widget *, void *)
{

  fltk_set_message();

  printf("After set message\n");

  fltk_set_destination();

  printf("After set destination\n");

  fltk_set_backoff();

  printf("After set Backoff\n");

  fltk_set_time();

  printf("After set time;\nDONE\n\n");
}


*/
Fl_Menu_Item menutable[] = {
  {"&Status timer", 0, 0, 0, FL_SUBMENU},
  {"set interval", 0, 0},
  {"Undo", 0, 0},
  {0},
  {0}
};

void *instance_search_thread_function(void *)
{
  start_instance_search();
}

/**
 * Items belonging on box but not on area are drawn, but seem impossible to interact
 */
int main()
{

  Fl_Window *G_win = new Fl_Window(1000, 510, "App"); //; G_win->set_modal();

  // Fl_Menu_Bar menubar (0,0,1000,30); menubar.menu(menutable);

  // Fl_Button *send = new Fl_Button(600, 300, 160, 35, "Send all data");
  // send->callback(send_all_data_sets);

  esp_interfaces_group = new Fl_Tabs(250, 50, 750, 460);

  on_instance_found_event(instance_found_action);
  on_instance_left_event(instance_left_action);


  // Create a pthread_t variable to store
  // thread ID
  pthread_t thread1;
  Fl::lock();
  // Creating a new thread.
  pthread_create(&thread1, NULL, instance_search_thread_function, NULL);

  // G_win->end();
  G_win->show();
  Fl::event_dispatch(exception_handler);
  int ret = Fl::run();

  printf("Closing connection\n");
  // connection_end();

  return ret;
}
