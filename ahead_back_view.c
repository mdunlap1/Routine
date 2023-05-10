/*******************************************************************************
 * ahead_back_view.c
 * Look ahead / back at upcoming items and items already completed. 
 *
 ******************************************************************************/
#include <stdlib.h>
#include <gtk/gtk.h>

#include "dates.h"
#include "helpers.h"
#include "main_enum.h"
#include "setup.h"
#include "sql_db.h"

#include <glib-object.h>


/* Since this view is loaded with two queries we keep a reference to them in 
 * this struct so that when the user modifies the view it can be successfully
 * re-loaded to reflect their changes. */
typedef struct view_data {
    char *range_desc;
    char *ahead_query;
    char *back_query;
    GtkListStore *ahead_model;
    GtkListStore *back_model;
} ViewData;
    
static ViewData capsule;

/* prototypes */
void set_ahead_back (GtkWidget *widget, char *ahead_query, char *back_query,
        char* range_desc);

/* These are used to setup the widgets for the view and connect callbacks */
static GtkWidget *make_ahead_back_view_box (char *ahead_query, char *back_query,
        char *range_desc);
static GtkWidget *make_connected_ahead_box (char *ahead_query);
static GtkWidget *make_connected_back_box (char *back_query);

/* add_upcoming_columns is used for upcoming items */ 
static void add_upcoming_columns (GtkTreeView *treeview);

/*add_hist_columns is used for previously completed items */
static void add_hist_columns (GtkTreeView *treeview);

/* updates the db upon user action and refreshes the current view */
static void change_hist_and_reload_view (GtkWidget *button, ViewData *capsule);
static void remove_hist_and_reload_view (GtkWidget *button, ViewData *capsule);
static void complete_and_reload_view (GtkWidget *button, ViewData *capsule);
static void snooze_and_reload_view (GtkWidget *button, ViewData *capsule);

/* these functions are used to facilitate transitions back and to main */
static void go_back (GtkWidget *button);
static void go_main (GtkWidget *button);
/* end prototypes */


/*
 * FUNC set_ahead_back
 *   Removes the current child of the toplevel window.
 * Uses the helper function make_ahead_back_view_box to create the ahead_back_view
 * and sets it in the toplevel window 
 *
 * If either ahead_query or back_query is NULL that part of the view
 * will be ignored. It should not occur that both are NULL 
 */
void set_ahead_back (GtkWidget *widget, char *ahead_query, char *back_query, char *range_desc)
{
    GtkWidget *window;
    GtkWidget *child;
    window = gtk_widget_get_toplevel (widget);
    child = gtk_bin_get_child (GTK_BIN (window));
    if (child != NULL) {
        gtk_widget_destroy (child);
    }

    /* set the capsules to facilitate re-loading the view */ 
    
    capsule.ahead_query = ahead_query;
    capsule.back_query  = back_query;
    capsule.range_desc = range_desc;

    GtkWidget *box;

    box = make_ahead_back_view_box (ahead_query, back_query, range_desc);

    gtk_container_add (GTK_CONTAINER (window), box);

    gtk_widget_show_all (window);
}

/*
 * FUNC make_ahead_back_view_box
 *   Helper function to set_ahead_back
 * Creates the ahead_back UI
 */
static GtkWidget *make_ahead_back_view_box (char *ahead_query, char *back_query,
        char *range_desc)
{
    GtkWidget *box,
              *box_ahead,
              *box_back,
              *label;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);


    label = gtk_label_new (range_desc);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    
    box_ahead = make_connected_ahead_box (ahead_query);
    box_back  = make_connected_back_box (back_query);

    if (box_ahead != NULL)
        gtk_box_pack_start (GTK_BOX (box), box_ahead, TRUE, TRUE, 10);
    if (box_back != NULL)
        gtk_box_pack_start (GTK_BOX (box), box_back, TRUE, TRUE, 10);
    if (box_ahead == NULL && box_back == NULL) {
        GtkWidget *label = gtk_label_new (NO_ITEMS_IN_RANGE);
        gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 10);
    }
        

        
    /* Buttons for the whole view */
    GtkWidget *button_box,
              *back,
              *main;
    button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    back = gtk_button_new_with_label (BACK);
    main = gtk_button_new_with_label (MAIN);

              

    gtk_box_pack_start (GTK_BOX (button_box),
                        back,
                        TRUE,
                        FALSE,
                        10);
    gtk_box_pack_start (GTK_BOX (button_box),
                        main,
                        TRUE,
                        FALSE,
                        10);

    /* callbacks */
    g_signal_connect (G_OBJECT (back), "clicked",
            G_CALLBACK (go_back), NULL);

    g_signal_connect (G_OBJECT (main), "clicked",
            G_CALLBACK (go_main), NULL);

    /* pack the rest and send it off */
    gtk_box_pack_start (GTK_BOX (box), button_box, FALSE, FALSE, 0);

    return box;
}

/*
 * FUNC add_upcoming_columns
 *   Used to add columns to the upcoming due items TreeView
 */
static void
add_upcoming_columns (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    /* column for select toggles */
    renderer = gtk_cell_renderer_toggle_new ();
    g_signal_connect (renderer, "toggled",
                    G_CALLBACK (checkbox_select_toggled), model);

    column = gtk_tree_view_column_new_with_attributes ("",
                                                     renderer,
                                                     "active",
                                                     COLUMN_SELECTED,
                                                     NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_append_column (treeview, column);


    /* column for description */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (DESCRIPTION,
                                                     renderer,
                                                     "text",
                                                     COLUMN_DESCRIPTION,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_DESCRIPTION);
    gtk_tree_view_append_column (treeview, column);

    /* column for user entered date */
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "editable", TRUE, NULL);
    g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), model);


    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_DATE_ENTRY));

    column = gtk_tree_view_column_new_with_attributes (COMPLETE_PUSH_BACK,
                                                     renderer,
                                                     "text",
                                                     COLUMN_DATE_ENTRY,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE_ENTRY);
    gtk_tree_view_append_column (treeview, column);

    /* column for category */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (CATEGORY,
                                                     renderer,
                                                     "text",
                                                     COLUMN_CATEGORY,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_CATEGORY);
    gtk_tree_view_append_column (treeview, column);

    /* column for date from date column of db*/
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (DUE,
                                                     renderer,
                                                     "text",
                                                     COLUMN_DATE_UNEDITABLE,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE_UNEDITABLE);
    gtk_tree_view_append_column (treeview, column);
    


}

/*
 * FUNC add_hist_columns
 *   Adds the columns to the history TreeView (displays items completed in the
 * user selected time window)
 */
static void
add_hist_columns (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    /* column for select toggles */
    renderer = gtk_cell_renderer_toggle_new ();
    g_signal_connect (renderer, "toggled",
                    G_CALLBACK (checkbox_select_toggled), model);

    column = gtk_tree_view_column_new_with_attributes ("",
                                                     renderer,
                                                     "active",
                                                     COLUMN_SELECTED,
                                                     NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_append_column (treeview, column);


    /* column for description */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (DESCRIPTION,
                                                     renderer,
                                                     "text",
                                                     COLUMN_DESCRIPTION,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_DESCRIPTION);
    gtk_tree_view_append_column (treeview, column);

    /* column for completion date*/
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (COMPLETED_ON,
                                                     renderer,
                                                     "text",
                                                     COLUMN_DATE_UNEDITABLE,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE_UNEDITABLE);
    gtk_tree_view_append_column (treeview, column);

    
    /* column for category */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (CATEGORY,
                                                     renderer,
                                                     "text",
                                                     COLUMN_CATEGORY,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_CATEGORY);
    gtk_tree_view_append_column (treeview, column);

    /* column for user to enter a date */
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "editable", TRUE, NULL);
    g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), model);


    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_DATE_ENTRY));

    column = gtk_tree_view_column_new_with_attributes (CHANGE_TO,
                                                     renderer,
                                                     "text",
                                                     COLUMN_DATE_ENTRY,
                                                     NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE_ENTRY);
    gtk_tree_view_append_column (treeview, column);

}

/* 
 * FUNC make_connected_ahead_box
 *   Helper function to make_ahead_back_view_box
 * Creates the box that holds items with due dates in the range selected
 * Connects the appropriate callbacks
 */
static GtkWidget *make_connected_ahead_box (char *ahead_query)
                                     
{
    int count;
    GtkWidget *box,
              *label,
              *sw,
              *treeview;
    GtkTreeModel *model;
             

    /* initilize dates */
    count = count_rows_from_query (ahead_query);
    if (count < 0) {
        fprintf(stderr, DATABASE_ERROR_FATAL);
        exit(EXIT_FAILURE);
    }
    if (count == 0)
        return NULL;

    /* The box that holds the whole view */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);

    /* Label the ahead portion */ 
    label = gtk_label_new (ITEMS_DUE_IN_RANGE);
    gtk_box_pack_start (GTK_BOX (box),
            label,
            FALSE,
            FALSE,
            0);

    
    /* args are Horizontal and veritcal aignments */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type ( GTK_SCROLLED_WINDOW (sw),
            GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
            GTK_POLICY_NEVER,
            GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);

    model = create_main_model_from_db (ahead_query);
    if (model == NULL) {
        fprintf(stderr, FATAL_ERROR);
        exit(EXIT_FAILURE);
    }

    treeview = gtk_tree_view_new_with_model (model);

    GtkTreeModel *tmodel;
    tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
    
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
            COLUMN_CATEGORY);

    g_object_unref (model);

    gtk_container_add (GTK_CONTAINER (sw), treeview);

    add_upcoming_columns (GTK_TREE_VIEW (treeview));

    /* buttons for the range...*/
    GtkWidget *ahead_button_box,
              *complete,
              *snooze,
              *space;
    ahead_button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    snooze = gtk_button_new_with_label (SNOOZE);
    space = gtk_label_new ("       ");
    complete = gtk_button_new_with_label (MARK_COMPLETED);
    gtk_box_pack_start (GTK_BOX (ahead_button_box),
                        space,
                        TRUE,
                        FALSE,
                        10);

    gtk_box_pack_start (GTK_BOX (ahead_button_box),
                        snooze,
                        FALSE,
                        FALSE,
                        10);


    gtk_box_pack_start (GTK_BOX (ahead_button_box),
                        complete,
                        FALSE,
                        FALSE,
                        10);

    gtk_box_pack_start (GTK_BOX (box), ahead_button_box,
                        FALSE, FALSE, 10);

    capsule.ahead_model = GTK_LIST_STORE (tmodel);

    /* set up callbacks */
    g_signal_connect (G_OBJECT (complete), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    g_signal_connect (G_OBJECT (snooze), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    g_signal_connect (G_OBJECT (complete), "clicked",
            G_CALLBACK (complete_and_reload_view), &capsule);

    g_signal_connect (G_OBJECT (snooze), "clicked",
            G_CALLBACK (snooze_and_reload_view), &capsule);


    return box;
}

/* 
 * FUNC make_conected_back_box
 *   Helper function to make_ahead_back_view_box
 * Creates the box that holds the items that have been completed in the date
 * range the user selected
 * Connects the appropriate callbacks
 */
static GtkWidget *make_connected_back_box (char *back_query)
                                     
{
    int count;
    GtkWidget *box,
              *label,
              *sw,
              *treeview;
    GtkTreeModel *model;
             

    count = count_rows_from_query (back_query);
    if (count < 0) {
        fprintf(stderr, DATABASE_ERROR_FATAL);
        exit(EXIT_FAILURE);
    }
    if (count == 0)
        return NULL;

    /* The box that holds the whole view */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);

    label = gtk_label_new (ITEMS_COMPLETED_IN_RANGE);
    gtk_box_pack_start (GTK_BOX (box),
            label,
            FALSE,
            FALSE,
            10);

    /* args are Horizontal and veritcal aignments */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type ( GTK_SCROLLED_WINDOW (sw),
            GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
            GTK_POLICY_NEVER,
            GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);

    model = create_main_model_from_db (back_query);
    if (model == NULL) {
        fprintf(stderr, FATAL_ERROR);
        exit(EXIT_FAILURE);
    }

    treeview = gtk_tree_view_new_with_model (model);

    GtkTreeModel *tmodel;
    tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

    capsule.back_model = GTK_LIST_STORE (tmodel);
    
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
            COLUMN_CATEGORY);


    gtk_container_add (GTK_CONTAINER (sw), treeview);

    add_hist_columns (GTK_TREE_VIEW (treeview));

    /* buttons for the range...*/
    GtkWidget *back_button_box,
              *change,
              *remove,
              *space;
    back_button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    remove = gtk_button_new_with_label (REMOVE);
    space = gtk_label_new ("       ");
    change = gtk_button_new_with_label (CHANGE_DATE);
    gtk_box_pack_start (GTK_BOX (back_button_box),
                        space,
                        TRUE,
                        FALSE,
                        10);

    gtk_box_pack_start (GTK_BOX (back_button_box),
                        remove,
                        FALSE,
                        FALSE,
                        10);


    gtk_box_pack_start (GTK_BOX (back_button_box),
                        change,
                        FALSE,
                        FALSE,
                        10);

    gtk_box_pack_start (GTK_BOX (box), back_button_box,
                        FALSE, FALSE, 10);

    /* callbacks */
    g_signal_connect (G_OBJECT (change), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    g_signal_connect (G_OBJECT (remove), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    g_signal_connect (G_OBJECT (change), "clicked",
            G_CALLBACK (change_hist_and_reload_view), &capsule);

    g_signal_connect (G_OBJECT (remove), "clicked",
            G_CALLBACK (remove_hist_and_reload_view), &capsule);

            
    g_object_unref (model);

    return box;
}

/* 
 * FUNC go_back
 *  Sends the user back to look_select_view
 */
static void go_back (GtkWidget *button)
{
    free (capsule.ahead_query);
    free (capsule.back_query);
    free (capsule.range_desc);
    set_look_select (button);
}

/*
 * FUNC go_main
 *   Sends the user back to the main_view
 */
static void go_main (GtkWidget *button)
{
    free (capsule.ahead_query);
    free (capsule.back_query);
    free (capsule.range_desc);
    set_main (button);
}


/*
 * FUNC remove_hist_and_reload_view 
 *   Wrapper function to remove_selected_historical_entries. Calls that function
 * then reloads the view.
 */
static void remove_hist_and_reload_view (GtkWidget *button, ViewData *capsule)
{
    GtkListStore *store = capsule->back_model;
    
    remove_selected_historical_entries (button, store);
    set_ahead_back (button, capsule->ahead_query, capsule->back_query, 
            capsule->range_desc);
}

/*
 * FUNC change_hist_and_reload_view 
 *   Wrapper function to change_hist_on_selected. Calls that function
 * then reloads the view.
 */
static void change_hist_and_reload_view (GtkWidget *button, ViewData *capsule)
{
    GtkListStore *store = capsule->back_model;
    
    change_hist_on_selected (button, store);
    set_ahead_back (button, capsule->ahead_query, capsule->back_query,
            capsule->range_desc);
}

/*
 * FUNC complete_and_reload_view 
 *   Wrapper function to complete_selected_items. Calls that function
 * then reloads the view.
 */
static void complete_and_reload_view (GtkWidget *button, ViewData *capsule)
{
    GtkListStore *store = capsule->ahead_model;
    complete_selected_items (button, store);
    set_ahead_back (button, capsule->ahead_query, capsule->back_query,
            capsule->range_desc);
}

/*
 * FUNC snooze_and_reload_view 
 *   Wrapper function to complete_selected_items. Calls that function
 * then reloads the view.
 */
static void snooze_and_reload_view (GtkWidget *button, ViewData *capsule)
{
    GtkListStore *store = capsule->ahead_model;
    snooze_selected_items (button, store);
    set_ahead_back (button, capsule->ahead_query, capsule->back_query,
            capsule->range_desc);
}
