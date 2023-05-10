/*******************************************************************************
 * look_select_view.c
 * Creates the look_select_view and associated callbacks.
 *
 ******************************************************************************/
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "dates.h"
#include "helpers.h"
#include "setup.h" 
#include "sql_db.h"

#define MAX_RANGE_OUT 365 /* arbitrary but okay for now */

typedef struct selection {
    /* for use with simple view selection */
    GtkComboBox   *direction; /*"ahead" or "back"*/
    GtkSpinButton *qty;       /* for use with selection.units */
    GtkComboBox   *units;     /* "days", "weeks", "months" */
    /* for use with specific date range selection */
    GtkTextBuffer *start;
    GtkTextBuffer *end;
} Selection;

static Selection selection;

/* Used to determine if the window of ahead_back_view needs to be split or not*/
static gchar *SPLIT =  "split";

/* prototypes */
void set_look_select (GtkWidget *widget);

/* helper for set_look_select */
static GtkWidget *make_look_select_view_box (void);


/* helpers for building look_select_view */
static GtkWidget *build_last_row (GtkWidget *box);
static GtkWidget *build_simple_row (GtkWidget *box);
static GtkWidget *build_custom_date_range_row
                 (GtkWidget *box, GtkWidget *start, GtkWidget *end);

/* parses the Selection struct to process the user's selection */
static void parse_selection (GtkWidget *button, gchar *split);

/* Uses malloc will need to be freed */
static char *make_query_from_to (char *tabel, char *sql_start_date, char *sql_end_date);

/* Uses malloc will need to be freed */
static char *make_query_simple (char *table, const char *direction, 
                         int qty, const char *units);
/* end prototypes */

/*
 * FUNC build_last_row
 *   Builds the last row of look_select_view
 * The last row contains a button to allow the user to go back to the main_view
 */
static GtkWidget *build_last_row (GtkWidget *box)
{
    GtkWidget *row;
    GtkWidget *back;
    back = gtk_button_new_with_label (BACK);
    row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_box_pack_start (GTK_BOX (row), back, FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (box), row, FALSE, FALSE, 10);

    return back;
}

/*
 * FUNC build_simple_row
 *   Builds the row for making simple ahead/back view selections
 * That is, for looking ahead one week, or back one month, etc...
 * Does NOT setup the callbacks
 *
 * Returns : the button that needs a callback attached to it
 */
static GtkWidget *build_simple_row (GtkWidget *box)
{
    GtkWidget     *row;
    GtkWidget     *look_label;
    GtkWidget     *direction;
    GtkAdjustment *adjustment;
    GtkWidget     *qty;
    GtkWidget     *units;
    GtkWidget     *button;

    row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    look_label = gtk_label_new (LOOK);
    direction = gtk_combo_box_text_new ();
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (direction), "ahead", AHEAD);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (direction), "back", BACK);
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (direction), "ahead");

    adjustment = gtk_adjustment_new (1,1, MAX_RANGE_OUT, 1, 5, 0);
    qty = gtk_spin_button_new (adjustment, 1.0, 0);

    units = gtk_combo_box_text_new ();
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (units), "days", DAYS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (units), "weeks", WEEKS);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (units), "months", MONTHS);
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (units), "weeks");

    button = gtk_button_new_with_label (VIEW);

    /* add relevant items to Selection */
    selection.direction = GTK_COMBO_BOX (direction);
    selection.qty       = GTK_SPIN_BUTTON (qty);
    selection.units      = GTK_COMBO_BOX (units);

    /* pack the widgets into the row */
    gtk_box_pack_start (GTK_BOX (row), look_label, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), direction, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), qty, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), units, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), button, TRUE, TRUE, 10);


    /* pack the row into the box */
    gtk_box_pack_start (GTK_BOX (box), row, FALSE, FALSE, 10);

    /* return the button to access for callbacks */
    return button;
}

/*
 * FUNC build_custom_date_range_row
 *   Builds the part of the UI that allows the user to select a custom date
 * range to look through for upcoming, or previously completed tasks
 * Does NOT set up callbacks
 *
 * Returns: the button that needs to have callbacks attached to it
 */
static GtkWidget *build_custom_date_range_row 
(GtkWidget *box, GtkWidget *start, GtkWidget *end)
{
    GtkWidget *row,
              *label_1,
              *label_2,
              *label_3,
              *button;

    row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    label_1 = gtk_label_new (FROM);
    label_2 = gtk_label_new (TO);
    label_3 = gtk_label_new (INCLUSIVE);

    gtk_label_set_xalign (GTK_LABEL (label_3), 0);

    GtkTextBuffer *start_buffer,
                  *end_buffer;

    start_buffer = gtk_text_buffer_new (NULL);
    end_buffer   = gtk_text_buffer_new (NULL);

    char *curr_date = get_current_date_str_in_user_frmt ();

    if (curr_date == NULL) {
        fprintf (stderr, MEM_FAIL_IN "look_select_view.c 1\n");
        exit (EXIT_FAILURE);
    }

    gtk_text_buffer_set_text (start_buffer, curr_date, strlen(curr_date));
    gtk_text_buffer_set_text (end_buffer, curr_date, strlen(curr_date));


    free (curr_date);

    start = gtk_text_view_new_with_buffer (start_buffer);
    end   = gtk_text_view_new_with_buffer (end_buffer);

    g_object_unref (start_buffer);
    g_object_unref (end_buffer);

    button = gtk_button_new_with_label (VIEW);

    /* add relevant items to Selection */
    selection.start = start_buffer;
    selection.end   = end_buffer;

    /* pack the widgets into the row */

    gtk_box_pack_start (GTK_BOX (row), label_1, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), start  , TRUE, TRUE, 10);
    gtk_box_pack_start (GTK_BOX (row), label_2, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), end    , TRUE, TRUE, 10);
    gtk_box_pack_start (GTK_BOX (row), label_3, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (row), button , FALSE, FALSE, 10);

    gtk_box_pack_start (GTK_BOX (box), row, TRUE, FALSE, 10);

    return button;
}

/*
 * FUNC make_look_select_view_box
 *   Helper function to set_look_select
 * Sets up the UI and attaches the callbacks
 */
static GtkWidget *make_look_select_view_box (void)
{
    /* box to hold view in */
    GtkWidget *box;
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* build the rows and put them in the box */ 
    GtkWidget *select_simple_view;
    select_simple_view = build_simple_row (box);

    GtkWidget   *start,
                *end,
                *select_from_to_view;

    select_from_to_view = build_custom_date_range_row (box, start, end);

    GtkWidget *back = build_last_row (box); 

    /* connect callbacks */
    g_signal_connect (G_OBJECT (back), "clicked",
            G_CALLBACK (set_main), NULL);
    g_signal_connect (G_OBJECT (select_from_to_view), "clicked",
            G_CALLBACK (parse_selection),  SPLIT);
    g_signal_connect (G_OBJECT (select_simple_view), "clicked",
            G_CALLBACK (parse_selection),  NULL);

    return box;
}

/*
 * FUNC set_look_select
 *   Removes the child widget from the toplevel window. Replaces it with the 
 * widget necessary for the look_select_view
 */
void set_look_select (GtkWidget *widget)
{
    GtkWidget *window;
    GtkWidget *child;
    window = gtk_widget_get_toplevel (widget);
    child = gtk_bin_get_child (GTK_BIN (window));
    if (child != NULL) {
        gtk_widget_destroy (child);
    }

    GtkWidget *box = make_look_select_view_box ();

    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (box));

    gtk_widget_show_all (window);
}

/* 
 * FUNC parse_selection
 *   parses the selection input by the user and hands it off to set_ahead_back
 * to take the user to the approprate view
 * 
 * Split is NOT null if parsing a from - to date range 
 */
static void parse_selection (GtkWidget *button, gchar *split)
{
    char *ahead_query,
         *back_query,
         *range_desc;

    /* For processing from to queries */
    if (split != NULL) {
        gchar *start_date = get_text_from_buffer (selection.start);
        gchar *end_date   = get_text_from_buffer (selection.end);

        if (start_date == NULL || end_date == NULL) {
            fprintf (stderr, MEM_FAIL_IN "look_select_view.c 2\n");
            exit (EXIT_FAILURE);
        }

        int valid_start_date,m,d,y;
        int valid_end_date,m2,d2,y2;
        valid_start_date = parse_and_validate_user_date_str(start_date, &m,&d,&y);
        valid_end_date   = parse_and_validate_user_date_str(end_date, &m2, &d2, &y2);
        
        
        if (valid_start_date && valid_end_date) {
            char *sql_s_date = make_date_str_sql_frmt (m,d,y);
            char *sql_e_date = make_date_str_sql_frmt (m2,d2,y2);

            /* Check that start_date <= end_date */
            int check_input;
            check_input = strcmp(sql_s_date, sql_e_date);
            if (check_input > 0) {
                error_dialog (button, START_BEFORE_END);
                free (sql_s_date);
                free (sql_e_date);
                g_free (start_date);
                g_free (end_date);
                return;
            }

            /* construct SQL query here and submit to constructor */
            ahead_query = make_query_from_to ("upcoming",
                                                    sql_s_date, 
                                                    sql_e_date);

            back_query = make_query_from_to ("history",
                                                   sql_s_date,
                                                   sql_e_date);

            if (ahead_query == NULL || back_query == NULL) {
                fprintf (stderr, MEM_FAIL_IN "look_select_view.c 3\n");
                exit (EXIT_FAILURE);
            }

            /* +3 is for spaces +1 is for NULL byte, however DATE_STR_LIMIT
             * already includes an extra byte, so we have more than we need */
            int size = strlen(FROM) + strlen(TO) + 2*DATE_STR_LIMIT + 3 + 1; 
            range_desc = malloc (sizeof(char) * size);

            if (range_desc == NULL) {
                fprintf (stderr, MEM_FAIL_IN "look_select_view.c 4\n");
                exit (EXIT_FAILURE);
            }

            sprintf (range_desc, "%s %s %s %s", FROM,
                                                start_date,
                                                TO,
                                                end_date);

            free (sql_s_date);
            free (sql_e_date);
            g_free (start_date);
            g_free (end_date);

        }
        else {
            error_dialog (button, INVALID_DATE\
                    DATE_FRMT_EXPLAIN);

            g_free (start_date);
            g_free (end_date);

            return;
        }

    }
    /* For processing simple queries */
    else {
        const gchar *direction;
        direction = gtk_combo_box_get_active_id (selection.direction);
        gint qty = gtk_spin_button_get_value_as_int (selection.qty);
        const gchar *units;
        units = gtk_combo_box_get_active_id (selection.units);
        ahead_query = make_query_simple ("upcoming", 
                                               direction, 
                                               qty, units); 
        back_query = make_query_simple ("history",
                                               direction,
                                               qty, units);

        if (ahead_query == NULL || back_query == NULL) {
            fprintf (stderr, MEM_FAIL_IN "look_select_view.c 5\n");
            exit (EXIT_FAILURE);
        }

        /* make description of range string here
         * format : ahead 1 day, back 2 weeks, etc */
        int size = 0;
        size += 3; // Since we can have at MOST a 3 digit numeric entry
        size += 1; // for NULL byte
        size += 2; // for spaces
        size += (strlen(AHEAD) > strlen(BACK)) ? strlen(AHEAD) : strlen(BACK);
        
        char * units_plural_or_non;
        if (qty == 1) {
            if (strcmp(units,DAYS) == 0)
                units_plural_or_non = DAY;
            else if (strcmp(units, WEEKS) == 0)
                units_plural_or_non = WEEK;
            
            else if (strcmp(units, MONTHS) == 0)
                units_plural_or_non = MONTH;
        }
        else {
            units_plural_or_non = (char*) units;
        }

        size += strlen(units_plural_or_non); // size of unit string

        range_desc = malloc (sizeof(char) * size);

        if (range_desc == NULL) {
            fprintf (stderr, MEM_FAIL_IN "look_select_view.c 6\n");
            exit (EXIT_FAILURE);
        }

        sprintf (range_desc, "%s %d %s", direction, qty, units_plural_or_non);

    }

    set_ahead_back (button, ahead_query, back_query, range_desc);
}

/* 
 * FUNC make_query_from_to
 *   Creates an SQL query for custom date range input by the user 
 * Ultimately this is handed off the set_ahead_back
 *
 * Returns NULL is memory allocation fails.
 *
 * NOTE: CALLS MALLOC WILL NEED TO BE FREED LATER
 */
static char *make_query_from_to (char *table, char *sql_start_date, char *sql_end_date)
{
    char *buffer;
    char *format = "SELECT * FROM %s WHERE date >= '%s' AND date <= '%s'";
    /* -6 is to remove chars from thre instances of %s in format */
    int len = strlen(format) - 6 + strlen(table);
    /* Add in the length of the date strings plus 1 for '\0' */
    len = len + 2 * strlen(sql_start_date) + 1;
    buffer = malloc (sizeof (char) * len );
    if (buffer == NULL) {
        return NULL;
    }
    int status = sprintf (buffer, format, table, sql_start_date, sql_end_date);
    if (status != len - 1) {
        free (buffer);
        return NULL;
    }
    return buffer;
}

/* 
 * FUNC make_query_simple
 *   Parses the users input from the simple_row to make an SQL query
 * Ultimately handed off to set_ahead_back
 *
 * Returns NULL is memory allocation fails.
 *
 * NOTE: CALLS MALLOC WILL NEED TO BE FREED LATER
 */
static char *make_query_simple (char *table, const char *direction, 
        int qty, const char *units)
{
    char d = (strcmp (direction,"ahead") == 0) ? '+' : '-';
    int actual_qty = (strcmp(units,"weeks") == 0) ? 7*qty : qty; 
    const char *actual_units = (strcmp(units,"weeks")==0) ? "days" : units;

    char *format;
    if (d == '+') {
        format = "SELECT * FROM %s WHERE date >= DATE('now','localtime','+1 "
            "days') AND date <= DATE('now','localtime', '+%d %s')";
    }
    else {
        format = "SELECT * FROM %s WHERE date <= DATE('now','localtime','-1 "
            "days') AND date >= DATE('now','localtime', '-%d %s')"; 
    }

    /* -6 for %s %d %s in format  +1 for '\0' */
    int len = strlen(format) - 6 + strlen(table) + (actual_qty / 10 + 1) + 
               strlen(actual_units) + 1;

    char *query = malloc (sizeof (char) * len); 

    if (query == NULL) {
        return NULL;
    }

    int res = sprintf (query, format, table, actual_qty, actual_units);

    if (res != len - 1) {
        free (query);
        return NULL;
    }

    return query;
}
