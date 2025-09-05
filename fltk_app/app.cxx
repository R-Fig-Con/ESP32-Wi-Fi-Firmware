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
 * Element that can cycle around every gui input screen
 * of every connected esp from instances_search
 *
 * When further level of knowledge of the fltk library is
 * achieved, attempt to create class to encompass properties
 * in a more readable manner
 */
Fl_Tabs *esp_interfaces_group;

Esp_Group::Esp_Group(char *address, int X, int Y, int W, int H) : Fl_Group(X, Y, W, H, "Text")
{

  // esp_interfaces_group->begin();

  this->ip_address = address;

  int current_y = Y + Y_SPACING;

  int x_start = X + 150;
  this->Time_group = new Fl_Group(x_start, current_y, X_SIZE + 100, Y_SIZE + 100, "Time group");
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
  if (!connection_start(address))
  {
    return;
  }

  // status s; //get_status(address, &s); // todo display starting info

  printf("REceived status\n");

  Fl::lock();
  esp_interfaces_group->begin();
  Esp_Group *g = new Esp_Group(
      address,
      esp_interfaces_group->x(),
      esp_interfaces_group->y() + 25, // following tabs-simple.cxx example
      esp_interfaces_group->w(),
      esp_interfaces_group->h());
  add_node(address, g);
  esp_interfaces_group->end();
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
  Fl::unlock();
}

Fl_Menu_Item menutable[] = {
    {"&Status timer", 0, 0, 0, FL_SUBMENU},
    {"set interval", 0, 0},
    {"Undo", 0, 0},
    {0},
    {0}};

void *instance_search_thread_function(void *)
{
  start_instance_search();

  return NULL;
}

/**
 * Items belonging on box but not on area are drawn, but seem impossible to interact
 */
int main() // valgrind --leak-check=full ./bin/app
{
  Fl::scheme("gtk+");
  Fl_Window *G_win = new Fl_Window(1000, 750, "App"); //; G_win->set_modal();

  // Fl_Menu_Bar menubar (0,0,1000,30); menubar.menu(menutable);

  esp_interfaces_group = new Fl_Tabs(50, 50, 750, 600);

  on_instance_found_event(instance_found_action);
  on_instance_left_event(instance_left_action);

  pthread_t search_thread;
  Fl::lock();
  // Creating a new thread.
  pthread_create(&search_thread, NULL, instance_search_thread_function, NULL);

  G_win->resizable(G_win);
  G_win->show();
  Fl::event_dispatch(exception_handler);
  int ret = Fl::run();

  return ret;
}

/*
  {
    Fl_Group *aaa = new Fl_Group(50,75,750-20,600-45,"Aaaewq");
      {
        // Put some different buttons into the group, which will be shown
        // when the tab is selected.
        Fl_Button *b1 = new Fl_Button(90, 100,90,25,"Button A1"); b1->color(88+1);
        Fl_Button *b2 = new Fl_Button(90, 130,90,25,"Button A2"); b2->color(88+2);
        Fl_Button *b3 = new Fl_Button(90,160,90,25,"Button A3"); b3->color(88+3);
      }
      aaa->end();

      // ADD THE "Bbb" TAB
      //   Same details as above.
      //
      Fl_Group *bbb = new Fl_Group(50,75,750-10, 600-35,"Bbbqwe");
      {
        // Put some different buttons into the group, which will be shown
        // when the tab is selected.
        Fl_Button *b1 = new Fl_Button( 90,100,90,25,"Button B1"); b1->color(88+1);
        Fl_Button *b2 = new Fl_Button(190,100,90,25,"Button B2"); b2->color(88+3);
        Fl_Button *b3 = new Fl_Button(290,100,90,25,"Button B3"); b3->color(88+5);
        Fl_Button *b4 = new Fl_Button( 90,130,90,25,"Button B4"); b4->color(88+2);
        Fl_Button *b5 = new Fl_Button(190,130,90,25,"Button B5"); b5->color(88+4);
        Fl_Button *b6 = new Fl_Button(290,130,90,25,"Button B6"); b6->color(88+6);
      }
      bbb->end();
  }
  esp_interfaces_group->end();

  */
