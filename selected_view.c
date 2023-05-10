/*******************************************************************************
 * selected_view.c
 * The view for an item that the user selected in edit_select_view 
 *
 ******************************************************************************/

#include <stdlib.h>
#include <gtk/gtk.h>

#include "dates.h"
#include "helpers.h"
#include "main_enum.h"
#include "setup.h"
#include "sql_db.h"

typedef struct attributes_widgets {
    char             *description;
    GtkTextBuffer    *completed_on;
    GtkTextBuffer    *next_due;
    GtkSpinButton    *freq;
    GtkComboBox      *freq_type;
    GtkEntry         *category;
} Attributes_widgets;

static Attributes_widgets attributes_selection; 

/* prototypes */
void set_selected_view (GtkWidget *widget, char *description);

/* Used to set up the view and connect callbacks */
static GtkWidget *make_selected_view_box (char *description);
static GtkWidget *make_connected_mark_completed_row (char *description);
static GtkWidget *make_connected_edit_attributes_box (char *description);
static GtkWidget *make_purge_section (void);
static GtkWidget *make_history_section (char *description);

/* helpers for make_history_section */
static void add_hist_columns (GtkTreeView *treeview);
static char *generate_hist_query (char *description);

/* wrapper function to return to edit_select: frees description */
static void back_to_edit_select (GtkWidget *button, char *description);

/* callbacks */
static void complete (GtkWidget *button , Attributes_widgets *attributes);
static void modify_due_date (GtkWidget *widget, Attributes_widgets *attributes);
static void modify_frequency (GtkWidget *widget, Attributes_widgets *attributes);
static void modify_category (GtkWidget *widget, Attributes_widgets *attributes);
static void remove_item_from_upcoming (GtkWidget *widget, Attributes_widgets *attributes);
static void purge_item (GtkWidget *widget, Attributes_widgets *attributes);
/* end prototypes */

/* 
 * FUNC generate_hist_query
 *   Creates the SQL query for accessing completed items
 * NOTE: USES MALLOC NEEDS TO BE FREED
 */
static char *generate_hist_query (char *description)
{
    char *frmt = "SELECT * FROM history WHERE description = '%s'"\
                  "ORDER BY date DESC";

    int len = strlen(frmt) + strlen(description) + 1;

    char *query = malloc (sizeof(char) * len);
    if (query == NULL) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 1\n");
        exit (EXIT_FAILURE);
    }

    int stat = sprintf(query, frmt, description);
    /* We check againast len - 3 because we are omiting '\0' and %s */
    if (stat != (len - 3)) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 2\n");
        exit (EXIT_FAILURE);
    }

    return query;
}
    

/*
 * FUNC add_hist_columns 
 *   Adds the columns to the history TreeView
 */ 
static void
add_hist_columns (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    /* column for select toggles */
    renderer = gtk_cell_renderer_toggle_new (); g_signal_connect (renderer, "toggled",
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
 * FUNC back_to_edit_select
 *   Takes the user back to edit_select_view
 * Frees description prior to exit.
 */
static void back_to_edit_select (GtkWidget *button, char *description)
{
    free (description);
    set_edit_select_view (button);
}

/*
 * FUNC make_history_section
 *   Makes the section that shows completion history of the selected item to the
 * user. Allows them remove completion entires and change completion dates.
 */
static GtkWidget *make_history_section (char *description)
{
    GtkWidget *box,
              *button_box,
              *remove_button,
              *change_date_button;

    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    button_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    remove_button = gtk_button_new_with_label (REMOVE);
    change_date_button = gtk_button_new_with_label (CHANGE_DATE);

    gtk_box_pack_start (GTK_BOX (button_box),
                        change_date_button,
                        FALSE, FALSE, 5);

    gtk_box_pack_start (GTK_BOX (button_box),
                        remove_button,
                        FALSE, FALSE, 5);

    int count;
    GtkWidget *sw,
              *treeview;
    GtkTreeModel *model;

    /* Note: memory allocation error handling handled in function */
    char *query = generate_hist_query (description);

    count = count_rows_from_query (query);
    if (count < 0) {
        fprintf(stderr, DATABASE_ERROR_FATAL);
        exit(EXIT_FAILURE);
    }
    if (count == 0) {
        GtkWidget *no_hist_label;
        no_hist_label = gtk_label_new ("No history for item");
        return no_hist_label;
    }


    /* args are Horizontal and veritcal aignments */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type ( GTK_SCROLLED_WINDOW (sw),
            GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
            GTK_POLICY_NEVER,
            GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);

    model = create_main_model_from_db (query);

    if (model == NULL) {
        fprintf(stderr, FATAL_ERROR);
        exit(EXIT_FAILURE);
    }

    treeview = gtk_tree_view_new_with_model (model);

    g_object_unref (model);

    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), COLUMN_CATEGORY);

    gtk_container_add (GTK_CONTAINER (sw), treeview);

    add_hist_columns (GTK_TREE_VIEW (treeview));

    
    gtk_box_pack_start (GTK_BOX (box),
                        button_box,
                        FALSE, FALSE, 10);

    GtkTreeModel *tmodel;
    tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

    g_signal_connect (G_OBJECT (remove_button), "clicked", 
            G_CALLBACK (remove_selected_historical_entries), tmodel);

    g_signal_connect (G_OBJECT (change_date_button), "clicked", 
            G_CALLBACK (change_hist_on_selected), tmodel);

    g_signal_connect (G_OBJECT (remove_button), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    g_signal_connect (G_OBJECT (change_date_button), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    free (query);
                        
    return box;

}



/* 
 * FUNC make_selected_view_box
 */
static GtkWidget *make_selected_view_box (char *description) 
{

    /* The box to return that holds the view */
    GtkWidget *box;
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* Add label telling the user what item the selected */
    GtkWidget *item_label;
    item_label = gtk_label_new (description);
    gtk_label_set_xalign (GTK_LABEL (item_label), 0);

    gtk_box_pack_start (GTK_BOX (box), item_label,
            FALSE, FALSE, 10);

    /* put all edit items into a box, then add this to a scroll view */
    GtkWidget *edit_box;
    edit_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *sw;
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
            GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
            GTK_POLICY_NEVER,
            GTK_POLICY_AUTOMATIC);

    
    /* Use separator */
    GtkWidget *separator0;
    separator0 = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        separator0,
                        FALSE, FALSE, 10);
    
    /* SECTION mark completed */

    /* Give row label and add to box */
    GtkWidget *completed_row_label;
    completed_row_label = gtk_label_new (UPDATE_COMPLETED);
    gtk_label_set_xalign (GTK_LABEL (completed_row_label), 0);
    gtk_box_pack_start (GTK_BOX (edit_box), completed_row_label,
            FALSE, FALSE, 10);

    /* make row */
    GtkWidget *mark_completed_row = make_connected_mark_completed_row (description);
    
    /* add row to box */
    gtk_box_pack_start (GTK_BOX (edit_box),
                        mark_completed_row,
                        FALSE, FALSE, 10);


    /* Place separator here in box */
    GtkWidget *separator1;
    separator1 = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        separator1,
                        FALSE, FALSE, 10);

    /* SECTION edit attributes */
    GtkWidget *attributes_label;
    attributes_label = gtk_label_new (UPDATE_ATTRIBUTES);
    gtk_label_set_xalign (GTK_LABEL (attributes_label), 0);

    gtk_box_pack_start (GTK_BOX (edit_box), 
                        attributes_label,
                        FALSE, FALSE, 10);

    GtkWidget *attributes_section = make_connected_edit_attributes_box (description);

    gtk_box_pack_start (GTK_BOX (edit_box),
                        attributes_section,
                        FALSE, FALSE, 10);

       
    GtkWidget *separator2;
    separator2 = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        separator2,
                        FALSE, FALSE, 10);

    /* SECTION purge from db */
    GtkWidget *purge_section_label;
    purge_section_label = gtk_label_new (REMOVE_AND_DELETE_HIST);
    gtk_label_set_xalign (GTK_LABEL (purge_section_label), 0);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        purge_section_label,
                        FALSE, FALSE, 10);

    GtkWidget *purge_row = make_purge_section ();

    gtk_box_pack_start (GTK_BOX (edit_box),
                        purge_row,
                        FALSE, FALSE, 10);

    GtkWidget *separator3;
    separator3 = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        separator3,
                        FALSE, FALSE, 10);

    /* SECTION history */
    GtkWidget *hist_label;
    hist_label = gtk_label_new ("History");
    gtk_label_set_xalign (GTK_LABEL (hist_label), 0);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        hist_label,
                        FALSE, FALSE, 10);

    GtkWidget *history_section;
    history_section = make_history_section (description);
    gtk_box_pack_start (GTK_BOX (edit_box),
                        history_section,
                        FALSE, FALSE, 10);

    gtk_container_add (GTK_CONTAINER (sw), edit_box);

    gtk_box_pack_start (GTK_BOX (box),
                        sw,
                        TRUE, TRUE, 10);

    GtkWidget *back;
    back = gtk_button_new_with_label (BACK);

    GtkWidget *bottom_row;
    bottom_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start (GTK_BOX (bottom_row), back, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (box), bottom_row,
            FALSE, FALSE, 10);

    g_signal_connect (G_OBJECT (back), "clicked", 
            G_CALLBACK (back_to_edit_select), description);


    return box;

}

/*
 * FUNC set_selected_view
 *   Removes the child widget from toplevel window
 * replaces it with the widget necessary for the selected_view
 */
void set_selected_view (GtkWidget *widget, char *description)
{
    GtkWidget *window,
              *child;
    window = gtk_widget_get_toplevel (widget);
    child = gtk_bin_get_child (GTK_BIN (window));
    if (child != NULL) {
        gtk_widget_destroy (child);
    }

    GtkWidget *box;
    box = make_selected_view_box (description);
    gtk_container_add (GTK_CONTAINER (window), box);

    gtk_widget_show_all (window);
}


/*
 * FUNC make_connected_mark_completed_row
 *   Makes the row of the UI that allows the user to mark item as complete
 * Connects the appropriate callbacks
 */
static GtkWidget *make_connected_mark_completed_row (char *description)
{

    Attributes_raw attributes;
    attributes.description = description;

    GtkWidget *completed_row,
              *completed_label,
              *mark_completed,
              *spacer;


    GtkWidget *completed_date;
    GtkTextBuffer *completed_date_buff; // not from GInitiallyUnowned

   
    completed_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    completed_label = gtk_label_new (COMPLETED_ON);
    completed_date_buff = gtk_text_buffer_new (NULL);
    attributes_selection.completed_on = completed_date_buff;
    completed_date = gtk_text_view_new_with_buffer (completed_date_buff);
    gtk_widget_set_size_request (completed_date, 100, 10); 
    mark_completed = gtk_button_new_with_label (MARK_COMPLETED);

    char *curr_date_str = get_current_date_str_in_user_frmt ();
    if (curr_date_str == NULL) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 3\n");
        exit (EXIT_FAILURE);
    }

    gtk_text_buffer_set_text (completed_date_buff,
                              curr_date_str,
                              -1);
    g_object_unref (completed_date_buff);
    free (curr_date_str);

    spacer = gtk_label_new ("       ");


    /* add widgets to row */
    gtk_box_pack_start (GTK_BOX (completed_row), 
                        completed_label,
                        FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (completed_row), 
                        completed_date,
                        FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (completed_row), 
                        spacer,
                        TRUE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (completed_row), 
                        mark_completed,
                        FALSE, FALSE, 10);

    g_signal_connect (G_OBJECT (mark_completed), "clicked",
            G_CALLBACK (complete), &attributes_selection);


    return completed_row;
}

/* 
 * FUNC make_connected_edit_attributes_box
 *   Makes part of UI dealing with editting of attributes
 * Connects the appropraite callbacks
 */
static GtkWidget *make_connected_edit_attributes_box (char *description)
{

    attributes_selection.description = description;

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* CHANGE DUE DATE */
    GtkWidget *due_row;
    GtkWidget *due_label,
              *due, 
              *change_due_date_button,
              *set_no_due_date_button,
              *spacer2;
    GtkTextBuffer *due_buffer; // not from GInitiallyUnowned

    due_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    due_label = gtk_label_new ("Next due date");

    Attributes_raw attributes;
    attributes.description = description;

    int load_success;
    load_success = load_rest_of_attributes_raw_from_desc (&attributes);
    if (load_success < 0) {
        fprintf(stderr, DATABASE_CANNOT_LOAD_ATTRIBUTES_FATAL);
        exit(EXIT_FAILURE);
    }

    due_buffer = gtk_text_buffer_new (NULL);
    attributes_selection.next_due= GTK_TEXT_BUFFER (due_buffer);

    char * due_date;
    if (strcmp (attributes.due_date , NOT_DUE) == 0)
        due_date = strdup (NOT_DUE);
    else
       due_date = convert_date_str_from_sql_to_user_frmt (attributes.due_date);

    if (due_date == NULL) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 4\n");
        exit (EXIT_FAILURE);
    }

    gtk_text_buffer_set_text (due_buffer, due_date, -1);
    free (due_date);
    due = gtk_text_view_new_with_buffer (due_buffer);
    g_object_unref (due_buffer);
    gtk_widget_set_size_request (due, 300, 10); 

    set_no_due_date_button = gtk_button_new_with_label (REMOVE_FROM_UPCOMING);
    change_due_date_button = gtk_button_new_with_label (CHANGE_DUE_DATE);

    spacer2 = gtk_label_new ("       ");


    gtk_box_pack_start (GTK_BOX (due_row), 
                        due_label,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (due_row), 
                        due,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (due_row), 
                        spacer2,
                        TRUE, TRUE, 10);

    gtk_box_pack_start (GTK_BOX (due_row), 
                        set_no_due_date_button,
                        FALSE, FALSE, 10);


    gtk_box_pack_start (GTK_BOX (due_row), 
                        change_due_date_button,
                        FALSE, FALSE, 10);


    g_signal_connect (G_OBJECT (change_due_date_button), "clicked",
            G_CALLBACK (modify_due_date), &attributes_selection);

    g_signal_connect (G_OBJECT (set_no_due_date_button), "clicked",
            G_CALLBACK (remove_item_from_upcoming), &attributes_selection);


    /* add row to box */
    gtk_box_pack_start (GTK_BOX (box),
                        due_row,
                        FALSE, FALSE, 10);


    /* CHANGE FREQUENCY */
    
    GtkWidget *freq_row,
              *freq_label,
              *frequency,
              *spacer3,
              *freq_type,
              *change_freq_button;

    freq_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    GtkAdjustment *adjustment;

    freq_label = gtk_label_new (EVERY);
    adjustment = gtk_adjustment_new (1,1,365,1,5,0);
    frequency  = gtk_spin_button_new (adjustment, 1.0, 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (frequency),
                               attributes.freq);

    freq_type = gtk_combo_box_text_new ();
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "days", DAYS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "weeks", WEEKS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "months", MONTHS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "years", YEARS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "no_repeat", NO_REPEAT);
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (freq_type), attributes.freq_type);

    change_freq_button = gtk_button_new_with_label (CHANGE_FREQ);

    spacer3 = gtk_label_new ("       ");

    gtk_box_pack_start (GTK_BOX (freq_row),
                        freq_label,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (freq_row),
                        frequency,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (freq_row),
                        freq_type,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (freq_row),
                        spacer3,
                        TRUE, TRUE, 10);
    gtk_box_pack_start (GTK_BOX (freq_row),
                        change_freq_button,
                        FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (box), 
                        freq_row,
                        FALSE, FALSE, 10);

    attributes_selection.freq = GTK_SPIN_BUTTON (frequency);
    attributes_selection.freq_type = GTK_COMBO_BOX (freq_type);

    g_signal_connect (G_OBJECT (change_freq_button), "clicked",
            G_CALLBACK (modify_frequency), &attributes_selection);


    free (attributes.due_date); // FREE!
    free (attributes.freq_type); // FREE!

    /* CHANGE CATEGORY */
    GtkWidget *cat_row,
              *cat_label,
              *category,
              *spacer_cat,
              *change_cat_button;
    GtkEntryCompletion *completion;


    cat_row   = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    cat_label = gtk_label_new (CATEGORY);

    category  = gtk_entry_new ();
    attributes_selection.category = GTK_ENTRY (category);
    completion = gtk_entry_completion_new ();
    gtk_entry_set_completion (GTK_ENTRY (category), completion);
    g_object_unref (completion);

    gtk_entry_set_text (GTK_ENTRY (category), attributes.category);

    char *cat_query = "select distinct category from attributes";
    GtkTreeModel * completion_model = create_completion_model_from_db (cat_query);
    if (completion_model == NULL) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 5\n");
        exit (EXIT_FAILURE);
    }

    gtk_entry_completion_set_model (completion, completion_model);
    g_object_unref (completion_model);
    gtk_entry_completion_set_text_column (completion, 0);

    change_cat_button = gtk_button_new_with_label (CHANGE_CATEGORY);
    spacer_cat = gtk_label_new ("       ");


    gtk_box_pack_start (GTK_BOX (cat_row),
                        cat_label,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (cat_row),
                        category,
                        FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (cat_row),
                        spacer_cat,
                        TRUE, TRUE, 10);

    gtk_box_pack_start (GTK_BOX (cat_row),
                        change_cat_button,
                        FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (box),
                        cat_row,
                        FALSE, FALSE, 10);

    g_signal_connect (G_OBJECT (change_cat_button), "clicked",
            G_CALLBACK (modify_category), &attributes_selection);

    free (attributes.category); // FREE!

    return box;
}

/* 
 * FUNC make_purge_section
 *   Makes the section to purge an item entirely from the program
 * Connects the necessary callbacks
 */
static GtkWidget *make_purge_section (void) 
{
    GtkWidget *purge_row,
              *purge_label_2,
              *purge_button,
              *purge_row_spacer;

    
    purge_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    purge_label_2 = gtk_label_new (PERMANENTLY_REMOVE);
    purge_row_spacer = gtk_label_new ("       ");
    purge_button = gtk_button_new_with_label (PURGE_ITEM);

    gtk_box_pack_start (GTK_BOX (purge_row),
                        purge_label_2,
                        FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (purge_row),
                        purge_row_spacer,
                        TRUE, TRUE, 10);
    gtk_box_pack_start (GTK_BOX (purge_row),
                        purge_button,
                        FALSE, FALSE, 10);

    g_signal_connect (G_OBJECT (purge_button), "clicked",
            G_CALLBACK (purge_item), &attributes_selection);

    return purge_row;
}

/* 
 * FUNC complete
 *   Marks items as complete...
 */
static void complete (GtkWidget *button , Attributes_widgets *attributes)
{
    char *description   = attributes->description;
    char *date_selected = get_text_from_buffer (attributes->completed_on);
    char *category      = (char *) gtk_entry_get_text (attributes->category);

    int m,d,y, rc;
    rc = parse_and_validate_user_date_str (date_selected, &m, &d, &y);
    if (rc == 0) {
        error_dialog (button, INVALID_DATE\
                               DATE_FRMT_EXPLAIN);
        g_free (date_selected);
        return;
    }

    char *sql_date = make_date_str_sql_frmt (m,d,y);
    if (sql_date == NULL) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 6\n");
        exit (EXIT_FAILURE);
    }

    int is_tracked =  get_tracking_from_db (description);
    
    if (is_tracked < 0) {
        error_dialog (button, DATABASE_FAILED_TO_GET_TRACKING);
        g_free (date_selected);
        free (sql_date);
        return;
    }
   

    int h_success = 1;
    int u_success = 1;

    if (is_tracked == 1)
        h_success = add_history (description, sql_date, (char *) category);

    if (h_success < 0) {
        error_dialog (button, DATABASE_MARK_COMPLETE_FAIL);
    }

    /* We only attempt to update due date if tracking was successful */
    if (h_success == 1) {
        u_success = update_due_date (description, sql_date);
        if (u_success < 0)
            error_dialog (button, DATABASE_UPDATE_DUE_DATE_FAIL);
    }

    if (u_success == 1 && h_success == 1)
        success_dialog (button, SUCCESS);

    free (sql_date);
    g_free (date_selected);


    /* refresh the view */
    set_selected_view (button, description);

}

/*
 * FUNC modify_due_date
 *   modifies the due date of the item 
 */
static void modify_due_date (GtkWidget *widget, Attributes_widgets *attributes)
{
    char *description = attributes->description;
    char *due_date = get_text_from_buffer (attributes->next_due);
    char *category = (char*) gtk_entry_get_text (attributes->category);
    int rc, m, d, y;
    rc = parse_and_validate_user_date_str (due_date, &m, &d, &y);
    if (rc == 0) {
        error_dialog (widget, INVALID_DATE\
                               DATE_FRMT_EXPLAIN);
        g_free (due_date);
        return;
    }

    char *sql_date = make_date_str_sql_frmt (m,d,y);
    if (sql_date == NULL) {
        fprintf (stderr, MEM_FAIL_IN "selected_view.c 7\n");
        exit (EXIT_FAILURE);
    }

    int change_status = change_db_due_date (description, sql_date, category);
    if (change_status < 0 ) 
        error_dialog (widget, DATABASE_UPDATE_DUE_DATE_FAIL);
    else
        success_dialog (widget, SUCCESS);

    free (sql_date);
    g_free (due_date);

    return;
}

/*
 * FUNC modify_frequency
 *   Changes the frequency with wich new due dates are assigned to the item
 */
static void modify_frequency (GtkWidget *widget, Attributes_widgets *attributes)
{
    char *description = attributes->description;
    int freq = gtk_spin_button_get_value (attributes->freq);
    const char *freq_type = gtk_combo_box_get_active_id (attributes->freq_type);

    int change_status = change_frequency (description, freq, freq_type);

    if (change_status < 0)
        error_dialog (widget, DATABASE_FREQ_FAIL);
    else
        success_dialog (widget, SUCCESS);
    
}


/*
 * FUNC modify_category
 *   Changes the category the item belongs to
 */
static void modify_category (GtkWidget *widget, Attributes_widgets *attributes)
{
    char *description = attributes->description;
    const char *category    = gtk_entry_get_text (attributes->category);
    gboolean has_apostrophes = check_for_apostrophes (category);
    if (has_apostrophes) {
        error_dialog (widget, CANNOT_HAVE_APOSTROPHES);
    }
    else {
        int change_success = 1;
        change_success = change_category (description, (char*) category);
        if (change_success == 1)
            success_dialog (widget, SUCCESS);
        else
            error_dialog (widget, DATABASE_FAILED_TO_CHANGE_CATEGORY);
    }
}

/* 
 * FUNC remove_item_from_upcoming
 *   Removes the item from the upcoming items
 */
static void remove_item_from_upcoming (GtkWidget *widget, Attributes_widgets *attributes)
{
    char *description = attributes->description;
    int remove_stat = 1;
    remove_stat = remove_from_upcoming (description);
    if (remove_stat < 0)
        error_dialog (widget, DATABASE_REMOVE_UPCOMING_FAIL);
    else {
        gtk_text_buffer_set_text (attributes->next_due, NOT_DUE, -1);

        success_dialog (widget, SUCCESS);
    }
}

/*
 * FUNC purge_item
 *   Removes the item from the database
 */
static void purge_item (GtkWidget *widget, Attributes_widgets *attributes)
{
    char *description = attributes->description;
    GtkWidget *dialog,
              *window;
    window = gtk_widget_get_toplevel (widget);
    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK_CANCEL,
             PURGE_WARN);

    int response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_OK)
    {
        int purge_stat = purge_permanently (description);
        if (purge_stat == 1) 
            success_dialog (widget, PURGE_SUCCESS);
        else {
            error_dialog (widget, DATABASE_PURGE_ITEM_FAIL);
        }
    }

    gtk_widget_destroy (dialog);

    back_to_edit_select (widget, description);

}
