/*******************************************************************************
 * edit_select_view.c
 * Creates the edit_select_view and associated callbacks.
 *
 ******************************************************************************/
#include <gtk/gtk.h>
#include <stdlib.h>

#include "setup.h" 
#include "sql_db.h"


#define NUMBER_OF_TYPES 5
#define IDX_OF_NO_REPEAT 4

/* these arrays are used to construct the model. They are necessary for 
 * language translation issues */
static char *indexer[NUMBER_OF_TYPES] = {"days", "weeks", "months", "years", 
    "no_repeat"};

static char *freq_types[NUMBER_OF_TYPES] = {DAYS, WEEKS, MONTHS, YEARS, 
    NO_REPEAT};


/* edit_select_view enum */
enum {
    COL_SELECTED = 0,
    COL_DESCRIPTION,
    COL_CATEGORY,
    COL_REPEAT,
    COL_TRACK_HIST,
    NUM_ATTR_COLUMNS
};

/* prototypes */
void set_edit_select_view (GtkWidget *widget);

static GtkWidget *make_edit_select_view_box (void);

static GtkWidget *make_connected_button_box (GtkTreeModel *model);

/* FIXME should this be in sql_db.c it is accessding the db */
static GtkTreeModel *create_attributes_model (void);

/* USES MALLOC WILL NEED TO USE FREE */
static char *make_repeat_string (int freq, const char *freq_type);

/* returns -1 only if something went terribly wrong */
static int find_index (const char * freq_type);

static void add_columns_for_edit_select_view (GtkTreeView *treeview);

/* radio button functions */
static void set_first_as_selected (GtkListStore *liststore);
static void clear_radio_buttons (GtkListStore *liststore);
static void
radio_select_toggled (GtkCellRendererToggle *cell,
               gchar                 *path_str,
               gpointer               data);

static void select_item_wrapper (GtkWidget *button, GtkTreeModel *model);

/* Memory allocation needs freed */ 
static char *select_item (GtkWidget *button, GtkTreeModel *model);
/* end prototypes */

/* 
 * FUNC clear_radio_buttons
 *   Clears all the radio buttons 
 * Note: modified from GTK tutorial 
 */
static void
clear_radio_buttons (GtkListStore *liststore)
{
  GtkTreeIter iter;

  gboolean valid;

  g_return_if_fail (liststore != NULL);

  /* Get first row in list store */
  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(liststore), &iter);

  gboolean selected = FALSE;

  while (valid)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (liststore), &iter,
                          COL_SELECTED, &selected,
                          -1);

      if (selected) {
          gtk_list_store_set (liststore, &iter,
                  COL_SELECTED, FALSE,
                  -1);
      }

      /* Make iter point to the next row in the list store */
      valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter);
    }

}


/*
 * FUNC radio_select_toggled
 *   Uses helper function clear_radio_buttons to unselect all radio buttons
 * then selects the one that was selected by the user.
 */
static void
radio_select_toggled (GtkCellRendererToggle *cell,
                      gchar                 *path_str,
                      gpointer               data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreeIter  iter;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    gboolean select;

    if (path == NULL) {
      fprintf (stderr, MEM_FAIL_IN "edit_select_view.c 1\n");
      exit (EXIT_FAILURE);
    }

    clear_radio_buttons (data);

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COL_SELECTED, &select, -1);

    /* do something with the value */
    select ^= 1;

    /* set new value */
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_SELECTED, select, -1);

    /* clean up */
    gtk_tree_path_free (path);
}


/*
 * FUNC add_columns_for_edit_select_view
 *   Adds columns to the TreeView for edit_select_view
 */
static void add_columns_for_edit_select_view (GtkTreeView *treeview)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel      *model;

    model = gtk_tree_view_get_model (treeview);


    /* setup COL_SELECTED */
    renderer = gtk_cell_renderer_toggle_new ();
    gtk_cell_renderer_toggle_set_radio (
            GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
    gtk_cell_renderer_toggle_set_activatable (
            GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);

    g_signal_connect (renderer, "toggled",
            G_CALLBACK (radio_select_toggled), model);

    column = gtk_tree_view_column_new_with_attributes ("",
                                                       renderer,
                                                       "active",
                                                       COL_SELECTED,
                                                       NULL);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
            GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);

    gtk_tree_view_append_column (treeview, column);

    /* setup COL_DESCRIPTION */

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (DESCRIPTION,
                                                       renderer,
                                                       "text",
                                                       COL_DESCRIPTION,
                                                       NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_DESCRIPTION);
    gtk_tree_view_append_column (treeview, column);

    /* setup COL_CATEGORY*/

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (CATEGORY,
                                                       renderer,
                                                       "text",
                                                       COL_CATEGORY,
                                                       NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_CATEGORY);
    gtk_tree_view_append_column (treeview, column);

    /* setup COL_REPEAT*/

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (REPEAT_EVERY,
                                                       renderer,
                                                       "text",
                                                       COL_REPEAT,
                                                       NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_REPEAT);
    gtk_tree_view_append_column (treeview, column);

    /* setup COL_TRACK_HIST*/

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (TRACK_HISTORY,
                                                       renderer,
                                                       "text",
                                                       COL_TRACK_HIST,
                                                       NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_TRACK_HIST);
    gtk_tree_view_append_column (treeview, column);

}


/* 
 * FUNC find_index
 *   Used to find the index of freq_type in indexer
 * This is useful as we intend the program to be able to support multiple
 * languages
 *
 * Returns : the index where the freq_type is found
 *           OR -1 only if something went terribly wrong 
 */
static int find_index (const char * freq_type)
{
    int i = 0;
    for (i = 0; i < NUMBER_OF_TYPES; i++) {
        if (strcmp (freq_type, indexer[i]) == 0)
            return i;
    }
    return -1;
}


/* 
 * FUNC make_repeat_string
 *   Makes the repeat string for the view in whatever language it was 
 * compiled with. 
 *
 * NOTE: USES MALLOC WILL NEED TO USE FREE
 */
static char *make_repeat_string (int freq, const char *freq_type)
{
    /* get index for language of freq type */
    int idx = find_index (freq_type);
    if (idx == -1) {
        fprintf (stderr, "IDX is -1 FAILURE\naborting\n");
        exit(EXIT_FAILURE);
    }

    char *type = freq_types[idx];

    int bytes_needed;
    
    if (idx != IDX_OF_NO_REPEAT) {
        /* Get the number of bytes for the freq */
        int num_digits = 0;
        int k = freq;
        while (k != 0) {
            k /= 10;
            num_digits++;
        }
        /* 2 is for ' ' and '\0' */
        bytes_needed = num_digits + strlen(type) + 2;
    }
    else {
        bytes_needed = strlen(type) + 1;
    }

    char *repeat = malloc (sizeof (char) * bytes_needed);
    if (repeat == NULL) {
        fprintf (stderr, MEM_FAIL_IN "edit_select_view.c 2\n");
        exit (EXIT_FAILURE);
    }

    int status;
    if (idx == IDX_OF_NO_REPEAT)
        status = sprintf(repeat, "%s", type);
    else
        status = sprintf(repeat, "%d %s", freq, type);

    if (status != bytes_needed - 1) {
        fprintf (stderr, MEM_FAIL_IN "edit_select_view.c 3\n");
        exit (EXIT_FAILURE);
    }

    return repeat;
}

/*
 * FUNC create_attributes_model
 *   Creates the data model for the TreeView of the view
 */
static GtkTreeModel *create_attributes_model (void)
{
    /* for sql queries */
    int count;
    int rc;
    sqlite3 *db = access_db ();
    sqlite3_stmt *res;
    char * query = "SELECT * FROM ATTRIBUTES ORDER BY description";

    rc = sqlite3_prepare (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        fprintf (stderr, DATABASE_ATTRIBUTES_MODEL_FAIL);
        exit (EXIT_FAILURE);
    }

    count = count_rows_of_res(db,res);
    if (count < 0) {
        fprintf (stderr, DATABASE_ATTRIBUTES_MODEL_FAIL);
        exit (EXIT_FAILURE);
    }

    GtkListStore *store;
    GtkTreeIter iter;

    store = gtk_list_store_new (NUM_ATTR_COLUMNS,
                                G_TYPE_BOOLEAN,  // selected
                                G_TYPE_STRING,   // description
                                G_TYPE_STRING,   // categroy
                                G_TYPE_STRING,   // repeat
                                G_TYPE_STRING);  // track hist

    const char *description;
    const char *category;
    int freq;
    const char *freq_type;
    int is_tracked;

    rc = sqlite3_step(res);

    for (int i = 0; i < count; i++)
    {
        description = sqlite3_column_text (res,0);
        category    = sqlite3_column_text (res,1);
        freq        = sqlite3_column_int  (res,2);
        freq_type   = sqlite3_column_text (res,3);
        is_tracked  = sqlite3_column_int  (res,4);

        /* Note memory allocation failures handled in make_repeat_string */
        char *repeat = make_repeat_string (freq, freq_type);
        char *track = (is_tracked == 1) ? YES : NO;

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COL_SELECTED,    FALSE,
                            COL_DESCRIPTION, description,
                            COL_CATEGORY,    category,
                            COL_REPEAT,      repeat, 
                            COL_TRACK_HIST,  track,
                            -1);
        free (repeat);
        rc = sqlite3_step (res);
    }

    if (count > 0) {
        set_first_as_selected (store);
    }

    sqlite3_finalize (res);

    return GTK_TREE_MODEL (store);
}

/*
 * FUNC select_item
 *   Finds the description of the item that the user selected and returns it
 *
 *   Note: Memory Allocation : returns from gtk_tree_model_get needs to be FREED
 *         consequently the char * returned here needs to be freed
 */
static char *select_item (GtkWidget *button, GtkTreeModel *model)
{
    GtkListStore *liststore = GTK_LIST_STORE (model);
    GtkTreeIter iter;

    gboolean valid;

    /* Get first row in list store */
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);

    gboolean selected = FALSE;
    char *description;

    while (valid)
    {

        gtk_tree_model_get (GTK_TREE_MODEL (liststore), &iter,
                            COL_SELECTED, &selected,
                            COL_DESCRIPTION, &description,
                            -1);

        if (selected) {
            return description;
        }
        else {
            free (description);
        }

        /* Make iter point to the next row in the list store */
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter);
    }
}

/*
 * FUNC select_item_wrapper
 *   Uses select_item to get the description of the item the user selected
 * Passes the description to set_selected_view
 */
static void select_item_wrapper (GtkWidget *button, GtkTreeModel *model)
{
    char *item = select_item (button, model);
    set_selected_view (button, item);
}

/*
 * FUNC make_connected_button_box
 *   Helper function to make_edit_select_view_box
 * Makes row that holds the back and select buttons AND connects the callbacks
 */
static GtkWidget *make_connected_button_box (GtkTreeModel *model)
{
    GtkWidget *row,
              *spacer,
              *back,
              *select;

    row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    back = gtk_button_new_with_label (BACK);
    select = gtk_button_new_with_label (SELECT);
    spacer = gtk_label_new ("       ");

    gtk_box_pack_start (GTK_BOX (row), back, FALSE, FALSE, 20);
    gtk_box_pack_start (GTK_BOX (row), spacer, TRUE, TRUE, 10);
    gtk_box_pack_start (GTK_BOX (row), select, FALSE, FALSE, 20);

    g_signal_connect (G_OBJECT (back), "clicked",
            G_CALLBACK (set_main), NULL);
    g_signal_connect (G_OBJECT (select), "clicked",
            G_CALLBACK (select_item_wrapper), model);


    return row;
}

/* 
 * FUNC make_edit_select_view_box
 *   Helper function to set_edit_select_view 
 * Sets up the UI. Makes use of make_connected_button_box
 */
static GtkWidget *make_edit_select_view_box (void)
{
    GtkWidget *box,
              *bottom_row;
    GtkTreeModel *tmodel;

    /* Box to hold the edit_select_view */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);


    GtkTreeModel *model;
    model = create_attributes_model ();

    GtkWidget *sw,
              *treeview;

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
            GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);

    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
            COL_CATEGORY);

    g_object_unref (model);

    gtk_container_add (GTK_CONTAINER (sw), treeview);

    add_columns_for_edit_select_view GTK_TREE_VIEW (treeview);

    tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

    /* bottom row button box */
    bottom_row = make_connected_button_box (tmodel);
    gtk_box_pack_start (GTK_BOX (box), bottom_row, FALSE, FALSE, 10);

    return box;
}


/* 
 * FUNC set_edit_select_view
 *   Removes the current child widget from the toplevel window and replaces it
 * with the edit_select_view widget. Uses the helper function make_edit_select_view_box
 */
void set_edit_select_view (GtkWidget *widget)
{
    GtkWidget *window;
    GtkWidget *child;
    window = gtk_widget_get_toplevel (widget);
    child = gtk_bin_get_child (GTK_BIN (window));
    if (child != NULL) {
        gtk_widget_destroy (child);
    }

    GtkWidget *box = make_edit_select_view_box ();

    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (box));

    gtk_widget_show_all (window);

}

/*
 * FUNC set_first_as_selected
 *   Sets the first item in the ListStore as selected
 */
static void
set_first_as_selected (GtkListStore *liststore)
{
  GtkTreeIter iter;

  gboolean valid;
  gboolean selected = FALSE;

  g_return_if_fail (liststore != NULL);

  /* Get first row in list store */
  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);

  if (valid)
    {

      gtk_tree_model_get (GTK_TREE_MODEL (liststore), &iter,
                          COL_SELECTED, &selected,
                          -1);

      gtk_list_store_set (liststore, &iter,
                          COL_SELECTED, TRUE,
                          -1);
    }
}
