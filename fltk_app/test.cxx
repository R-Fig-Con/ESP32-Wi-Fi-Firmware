#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Choice.H>

/**
 * note: redraw seems to work to already existing items
 * 
 * If it still did not exist, redrawing the mother group such as the window
 * should work
 */

#define FIRST_CHOICE "first"
#define SECOND_CHOICE "second"
#define THIRD_CHOICE "third"

#define F_ID 0
#define S_ID 1
#define T_ID 2

#define BOX_X_START 300
#define BOX_Y_START 300

// Globals
Fl_Window *G_win = NULL;
Fl_Choice *G_choice = NULL;



//Fl_Group* grp = NULL;

static void first_view(){
    new Fl_Input(BOX_X_START, BOX_Y_START, 75, 25, "First viw:");
}

static void second_view(){
    new Fl_Input(BOX_X_START, BOX_Y_START, 75, 25, "Second view:");
}

static void third_view(){
    new Fl_Input(BOX_X_START, BOX_Y_START, 75, 25, "Third: aasd");
}

// Fl_Choice callback for changing chart type()
static void chart_type_cb(Fl_Widget *w, void*) {
    const Fl_Menu_Item *item = G_choice->mvalue();  // item picked
    G_win->begin();
    switch (item->argument()){
    case 0:
        printf("First view draw\n");
        first_view();
        G_choice->position(200, 50);
        break;
    
    case 1:
        printf("Second view draw\n");
        second_view();
        G_choice->position(250, 50);
        break;

    case 2:
        printf("Third view draw\n");
        third_view();
        G_choice->position(300, 50);
        break;
    
    default:
        break;
    }
    G_win->redraw();
    G_win->end();
    //grp->redraw();
    printf("Choice: '%s', argument=%ld\n", G_choice->text(), item->argument());
}

// main
int main() {

    G_win = new Fl_Window(1000, 510, "Fixed choice");
    //G_win->begin();

    //grp = new Fl_Group(BOX_X_START, BOX_Y_START, 100, 50); grp->begin();


    // Let user change chart type
    G_choice = new Fl_Choice(250, 50, 200,25,"Choose view:");
    G_choice->add(FIRST_CHOICE,  0, chart_type_cb, (void*) F_ID);
    G_choice->add(SECOND_CHOICE, 0, chart_type_cb, (void*) S_ID);
    G_choice->add(THIRD_CHOICE,  0, chart_type_cb, (void*) T_ID);
    G_choice->value(T_ID);
    G_win->resizable(G_win);
    //G_win->end();
    G_win->show();
    return(Fl::run());
}
