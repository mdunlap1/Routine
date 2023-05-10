/*******************************************************************************
 * setup.h
 * Basic ptototypes necessary to set up views in program.
 *
 ******************************************************************************/
/* Choose language to compile program in here. 
 * See corresponding language file for date format selection */
#include "text_en.h" 
//#include "es_text.h"

/* Functions to set up the various views*/
void set_main (GtkWidget *widget);
void set_add (GtkWidget *button);
void set_look_select (GtkWidget *widget);
void set_ahead_back (GtkWidget *widget, char *ahead_query, char *back_query,
        char* range_desc);
void set_edit_select_view (GtkWidget *widget);
void set_selected_view (GtkWidget *widget, char *description);
