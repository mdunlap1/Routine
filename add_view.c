/*******************************************************************************
 * add_view.c
 * Creates the add_view for the program. Sets up the callbacks.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "dates.h"
#include "helpers.h"
#include "setup.h"
#include "sql_db.h"

/* Used to aggregate attributes from add_view.c for processing in sql_db.c */
typedef struct attributes {
    GtkEntry      *description; 
    GtkTextBuffer *date; 
    GtkSpinButton *freq;
    GtkComboBox   *freq_type;
    GtkEntry      *category;
    GtkComboBox   *track_history;
} Attributes;

 
static Attributes attributes;
static char *desc_query = "select distinct description from attributes";
static char *cat_query = "select distinct category from attributes";

/* prototypes */
void set_add(GtkWidget* button);

static GtkWidget *make_add_view_box(void);

static void submit_new_item (GtkWidget *widget, Attributes *attributes);

static void reset_completion (GtkEntry *entry, char *entry_query);

/* end of prototypes */

/*
 * FUNC submit_new_item
 *   Submits the item that the user input to the db
 * Checks the item for errors and alerts the user if any found
 */
static void submit_new_item (GtkWidget *widget, Attributes *attributes)
{
    const gchar *description = gtk_entry_get_text (attributes->description);
    gchar *date_str    = get_text_from_buffer (attributes->date);
    gint frequency     = gtk_spin_button_get_value_as_int (attributes->freq);
    const gchar *freq_type = gtk_combo_box_get_active_id (attributes->freq_type);
    const gchar *category    = gtk_entry_get_text (attributes->category);
    const gchar *track_hist = gtk_combo_box_get_active_id (attributes->track_history);

    int m,d,y;
    int in_use = desc_already_in_use (description);

    int valid_date = parse_and_validate_user_date_str (date_str, &m, &d,&y);

    gboolean desc_has_apost = check_for_apostrophes (description);
    gboolean cat_has_apost = check_for_apostrophes (category);

    if (!in_use && valid_date && !desc_has_apost && !cat_has_apost) {

        char * sql_date_str = make_date_str_sql_frmt (m,d,y);
        if (sql_date_str == NULL) {
            fprintf (stderr, MEM_FAIL_IN "add_view.c 1\n");
            exit(EXIT_FAILURE);
        }

        int add_success = 1;
        int up_success = 1;

        add_success = add_attributes (description, category, frequency, 
                                      freq_type, track_hist);
        if (add_success < 0)
            error_dialog (widget, DATABASE_ADD_FAIL);

        if (add_success == 1) {
            up_success = add_upcoming (description, sql_date_str, category);
            if (up_success < 0)
                error_dialog (widget, DATABASE_ADD_UPCOMING_FAIL);
        }

        if (add_success == 1 && up_success == 1)
            success_dialog (widget, SUCCESS);

        free (sql_date_str);

    }

    /* Handle error message popups from bad input */
    else {
        if (in_use > 0) {
            error_dialog (widget, DESCRIPTION_COLLISION);
        }
        else if (in_use < 0) {
            error_dialog (widget, DATABASE_ERROR);
        }
        else if (!valid_date) {
            error_dialog (widget, INVALID_DATE\
                                   DATE_FRMT_EXPLAIN);
        }
        else if (cat_has_apost || desc_has_apost) {
            error_dialog (widget, CANNOT_HAVE_APOSTROPHES);
        }

    }


    g_free (date_str);

    /* Reset completions here */
    reset_completion (attributes->description, desc_query);
    reset_completion (attributes->category, cat_query);
   
    return;

}

/*
 * FUNC make_add_view_box
 *   Helper function to set_add
 * Sets up the box holding the widgets for the add_view and sets the callbacks
 */
static GtkWidget *make_add_view_box(void)
{
    GtkWidget *box;
    GtkWidget *grid;

    GtkWidget *desc_label;
    GtkWidget *desc;

    GtkWidget *due_label;
    GtkWidget *due_date;
    GtkTextBuffer *due_date_buffer; // not from GInitiallyUnowned

    GtkWidget *freq_label;
    GtkWidget *frequency;
    GtkAdjustment *adjustment;
    GtkWidget *freq_type;

    GtkWidget *cat_label;
    GtkWidget *category;
    GtkTextBuffer *cat_buffer; // not from GInitiallyUnowned

    GtkWidget *yes_no_label;
    GtkWidget *yes_no;

    GtkWidget *back;
    GtkWidget *submit;

    /* box to hold the add_view in */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);

    grid = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (grid), 10);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 10);

    gtk_box_pack_start (GTK_BOX (box), grid, TRUE, TRUE, 10);


    /* row 0 description field*/
    desc_label = gtk_label_new (DESCRIPTION);
    gtk_label_set_xalign (GTK_LABEL (desc_label), 0);
    desc = gtk_entry_new ();
    GtkEntryCompletion *desc_completion;
    desc_completion = gtk_entry_completion_new ();
    gtk_entry_set_completion (GTK_ENTRY (desc), desc_completion);
    g_object_unref (desc_completion);

    GtkTreeModel * desc_completion_model = create_completion_model_from_db (desc_query);

    if (desc_completion_model == NULL) {
        fprintf (stderr, MEM_FAIL_IN "add_view.c 2\n");
        exit (EXIT_FAILURE);
    }

    gtk_entry_completion_set_model (desc_completion, desc_completion_model);
    g_object_unref (desc_completion_model);

    gtk_entry_completion_set_text_column (desc_completion, 0);
 
    attributes.description = GTK_ENTRY (desc);


    gtk_grid_attach (GTK_GRID (grid), desc_label,
                    //colPos    rowPos  colSpan rowSpan
                        0,        0,       1,      1);

    gtk_grid_attach (GTK_GRID (grid), desc,
                    //colPos    rowPos  colSpan rowSpan
                        1,        0,       5,      1);
                 

    /* row 1 due date field */
    due_label = gtk_label_new (DUE_ON);
    due_date_buffer = gtk_text_buffer_new (NULL);
    due_date = gtk_text_view_new_with_buffer (due_date_buffer);
    g_object_unref (due_date_buffer);

    char *date_str = get_current_date_str_in_user_frmt ();
    if (date_str == NULL) {
        fprintf (stderr, MEM_FAIL_IN "add_view.c 3\n");
        exit (EXIT_FAILURE);
    }

    gint len = strlen(date_str);
    gtk_text_buffer_set_text (due_date_buffer, date_str, len);

    free(date_str);

    attributes.date = due_date_buffer;

    gtk_label_set_xalign (GTK_LABEL (due_label), 0);

    gtk_grid_attach (GTK_GRID (grid), due_label,
                    //colPos    rowPos  colSpan rowSpan
                        0,        1,       1,      1);

    gtk_grid_attach (GTK_GRID (grid), due_date,
                    //colPos    rowPos  colSpan rowSpan
                        1,        1,       1,      1);
   

    /* row 2 frequency fields*/
    freq_label = gtk_label_new (FREQUENCY);
    adjustment = gtk_adjustment_new (1,1,365,1,5,0);
    frequency  = gtk_spin_button_new (adjustment, 1.0, 0);

    attributes.freq = GTK_SPIN_BUTTON (frequency);

    gtk_label_set_xalign (GTK_LABEL (freq_label), 0);

    freq_type = gtk_combo_box_text_new ();
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "days", DAYS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "weeks", WEEKS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "months", MONTHS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "years", YEARS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (freq_type), "no_repeat", NO_REPEAT);
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (freq_type), "days");

    attributes.freq_type = GTK_COMBO_BOX (freq_type);

    gtk_grid_attach (GTK_GRID (grid), freq_label,
                    //colPos    rowPos  colSpan rowSpan
                        0,        2,       1,      1);
    gtk_grid_attach (GTK_GRID (grid), frequency,
                    //colPos    rowPos  colSpan rowSpan
                        1,        2,       1,      1);

    gtk_grid_attach (GTK_GRID (grid), freq_type,
                    //colPos    rowPos  colSpan rowSpan
                        2,        2,       1,      1);


    /* row 3 category selection */
    cat_label = gtk_label_new (CATEGORY);
    category = gtk_entry_new ();
    GtkEntryCompletion *completion;
    completion = gtk_entry_completion_new ();
    gtk_entry_set_completion (GTK_ENTRY (category), completion);
    g_object_unref (completion);

    GtkTreeModel * completion_model = create_completion_model_from_db (cat_query);
    gtk_entry_completion_set_model (completion, completion_model);
    g_object_unref (completion_model);

    gtk_entry_completion_set_text_column (completion, 0);
 

    attributes.category = GTK_ENTRY (category);

    gtk_label_set_xalign (GTK_LABEL (cat_label), 0);

    gtk_grid_attach (GTK_GRID (grid), cat_label,
                    //colPos    rowPos  colSpan rowSpan
                        0,        3,       1,      1);
    gtk_grid_attach (GTK_GRID (grid), category,
                    //colPos    rowPos  colSpan rowSpan
                        1,        3,       4,      1);

    /* row 4 track history slection */
    yes_no_label = gtk_label_new (TRACK_HISTORY);
    yes_no = gtk_combo_box_text_new ();
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (yes_no), "y", YES);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (yes_no), "n", NO);
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (yes_no), "y");

    attributes.track_history = GTK_COMBO_BOX (yes_no);


    gtk_label_set_xalign (GTK_LABEL (yes_no_label), 0);

    gtk_grid_attach (GTK_GRID (grid), yes_no_label,
                    //colPos    rowPos  colSpan rowSpan
                        0,        4,       1,      1);
    gtk_grid_attach (GTK_GRID (grid), yes_no,
                    //colPos    rowPos  colSpan rowSpan
                        1,        4,       1,      1);


    /* buttons at the bottom of the box */
    back = gtk_button_new_with_label (BACK);
    submit = gtk_button_new_with_label (SUBMIT);

    GtkWidget *p1,
              *p2;
    p1 = gtk_label_new ("     ");
    p2 = gtk_label_new ("     ");

    GtkWidget *bottom_row;
    bottom_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start (GTK_BOX (bottom_row), back, TRUE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (bottom_row), p1, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (bottom_row), p2, TRUE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (bottom_row), submit, TRUE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (box), bottom_row, TRUE, FALSE, 0);
    
   
    /* callbacks */ 
    g_signal_connect (G_OBJECT (back), "clicked", 
            G_CALLBACK (set_main), NULL);

    g_signal_connect (G_OBJECT (submit), "clicked",
            G_CALLBACK (submit_new_item), &attributes);

    return box;
}

/*
 * FUNC set_add
 *   Removes the current view and replaces it with the add view.
 */
void set_add(GtkWidget* button)
{
    GtkWidget *window;
    window = gtk_widget_get_toplevel (button);
    if (window == NULL) {
        fprintf (stderr, MEM_FAIL_IN "add_view.c 4\n");
        exit (EXIT_FAILURE);
    }

    GtkWidget *child;
    child = gtk_bin_get_child (GTK_BIN (window));
    gtk_widget_destroy (child);

    GtkWidget *box = make_add_view_box ();

    gtk_container_add (GTK_CONTAINER (window), box);

    gtk_widget_show_all (window);
}

/* 
 * FUNC reset_completion 
 *   Resets the completions for Category and Description.
 * Uses a SQL query to get the new completion
 *
 * Inputs:
 *   GtkEntry *entry : the GtkEntry whose completion needs updated
 *   char *entry_query : the SQL query to get the updated data
 */
static void reset_completion (GtkEntry *entry, char *entry_query) 
{
    GtkEntryCompletion *entry_completion = gtk_entry_get_completion (entry);
    GtkTreeModel *model = gtk_entry_completion_get_model (entry_completion);
    
    /*
    if (model != NULL) {
        g_object_unref (model);
    }
    */
    

    GtkTreeModel * entry_completion_model = create_completion_model_from_db (entry_query);

    if (entry_completion_model == NULL) {
        fprintf (stderr, MEM_FAIL_IN "add_view.c 5\n");
        exit (EXIT_FAILURE);
    }

    gtk_entry_completion_set_model (entry_completion, entry_completion_model);
    g_object_unref (entry_completion_model);
}
