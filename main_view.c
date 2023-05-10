/*******************************************************************************
 * main_view.c
 * Creates the main_view for the program and sets up the callbacks.
 *
 ******************************************************************************/
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "dates.h"
#include "helpers.h"
#include "main_enum.h"
#include "setup.h"
#include "sql_db.h"

/* prototypes */
void set_main (GtkWidget *widget);

static GtkWidget *make_main_view_box ();

static void add_columns (GtkTreeView *treeview);

static void act_on_selected (GtkWidget *button, GtkListStore *store);
/* end of prototypes */


/* 
 * FUNC act_on_selected
 *   Walks through the GtkListStore and takes actions on selected items.
 * Actions handled are MARK_COMPLETED or SNOOZE functionality
 *
 * Input: GtkWidget *button : the button pressed 
 *        GtkListStore *store : the data to be processed
 *
 * Note: Modified from GTK tutorial 
 */
static void act_on_selected (GtkWidget *button, GtkListStore *store)
{
  GList *rr_list = NULL;    /* list of GtkTreeRowReferences to remove */
  GList *node;              /* used to walk through rr_list           */
  gchar *description;       /* description of item that was selected  */
  gchar *date_str;          /* date entered for action                */
  gchar *category;          /* category for the item selected         */
  const gchar* button_name; /* name of the button pressed             */

  /* Get the name of the button that was pressed 
   * (button will determine what actions to take */
  button_name = gtk_button_get_label (GTK_BUTTON (button));

  /* Aggregate all the items that were selected 
   * (these are the items that will be acted on 
   * WARNING: gtk_tree_model_foreach is deprecated as of version 4.1*/
  gtk_tree_model_foreach(GTK_TREE_MODEL(store),
                         (GtkTreeModelForeachFunc) collect_selected,
                         &rr_list);

  /* Walk through the list and take the appropriate actions */
  for (node = rr_list;  node != NULL;  node = node->next)
    {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)node->data);

      if (path)
        {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
            gtk_tree_model_get (GTK_TREE_MODEL (store),
                                &iter,
                                COLUMN_DESCRIPTION, &description,
                                COLUMN_DATE_ENTRY, &date_str,
                                COLUMN_CATEGORY, &category,
                                -1);
            int stat,m,d,y;
            stat = parse_and_validate_user_date_str ((char*) date_str, &m, &d, &y);

            if (stat == 0) {
                error_dialog (button, INVALID_DATE\
                                       DATE_FRMT_EXPLAIN);
                /* Free resources before continuing */
                gtk_tree_path_free(path);
                free (description);
                free (date_str);
                free (category);
                continue;
            }

            char *sql_date = make_date_str_sql_frmt (m,d,y);
            if (sql_date == NULL) {
                fprintf (stderr, MEM_FAIL_IN "main_view.c 1\n");
                exit (EXIT_FAILURE);
            }

            int is_tracked = get_tracking_from_db ((char *) description);

            if (is_tracked < 0) {
                error_dialog (button, DATABASE_FAILED_TO_GET_TRACKING);

                /* Free resources before continuing */
                free (sql_date);
                gtk_tree_path_free(path);
                free (description);
                free (date_str);
                free (category);

                continue;
            }

            if (strcmp(button_name, MARK_COMPLETED) == 0) {

                int hist_success = 1;
                int update_success = 1;

                if (is_tracked == 1)
                    hist_success = add_history (description, sql_date, category);
                if (hist_success < 0) {
                    error_dialog (button, DATABASE_MARK_COMPLETE_FAIL);

                    free (description);
                    free (date_str);
                    free (category);
                    free (sql_date);
                    gtk_tree_path_free(path);

                    continue;
                }

                update_success = update_due_date (description, sql_date);

                if (update_success < 0) {
                    error_dialog (button, DATABASE_UPDATE_DUE_DATE_FAIL);

                    free (description);
                    free (date_str);
                    free (category);
                    free (sql_date);
                    gtk_tree_path_free(path);

                    continue;
                }
                
            }
            else {
                int push_back_success = 1;
                push_back_success = push_back_upcoming (description, sql_date);
                if (push_back_success < 0) {
                    error_dialog (button, DATABASE_SNOOZE_FAIL);

                    free (description);
                    free (date_str);
                    free (category);
                    free (sql_date);
                    gtk_tree_path_free(path);

                    continue;
                }
            }

            free (description);
            free (date_str);
            free (category);
            free (sql_date);
            
            gtk_list_store_remove(store, &iter);
          }

          gtk_tree_path_free(path);
        }
    }

  g_list_free_full(rr_list, (GDestroyNotify)gtk_tree_row_reference_free);
}

/* 
 * FUNC add_columns 
 *   Add columns to main_view
 */
static void add_columns (GtkTreeView *treeview)
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
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column (treeview, column);

    /* column for user to enter a date */
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

    /* column for due date*/
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
 * FUNC make_main_view_box
 *   Helper function to set_main
 * Creates all the widgets and sets all the callbacks needed for the main_view
 */
static GtkWidget *make_main_view_box ()
{

    GtkWidget *box,
              *label,
              *sw,
              *treeview;
    GtkTreeModel *model;
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    label = gtk_label_new (TODAY_VIEW_HEADING);
    gtk_box_pack_start (GTK_BOX (box),
                        label,
                        FALSE,
                        FALSE,
                        10);

    /* set up scrolled window */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                           GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                      GTK_POLICY_NEVER,
                                      GTK_POLICY_AUTOMATIC);
    /* add to box */
    gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);

    char *query = "SELECT * FROM upcoming WHERE date <= DATE('now','localtime')";

    model = create_main_model_from_db (query);
    if (model == NULL) {
        fprintf(stderr, FATAL_ERROR);
        exit(EXIT_FAILURE);
    }

    /* create tree view */
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                       COLUMN_DESCRIPTION);

    g_object_unref (model);

    GtkTreeModel *tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

    gtk_container_add (GTK_CONTAINER (sw), treeview);

    /* add columns to the tree view */
    add_columns (GTK_TREE_VIEW (treeview));

    /* add buttons to the bottom of the view */
    GtkWidget *button_box;
    button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *add_button,
              *edit_search_button,
              *look_ahead_or_back_button,
              *snooze_button,
              *mark_completed_button;

    add_button = gtk_button_new_with_label (ADD);
    g_signal_connect (G_OBJECT (add_button), "clicked",
            G_CALLBACK (set_add), NULL);

    edit_search_button = gtk_button_new_with_label (EDIT_SEARCH);

    look_ahead_or_back_button = gtk_button_new_with_label (LOOK_AHEAD_OR_BACK);

    snooze_button = gtk_button_new_with_label (SNOOZE);

    mark_completed_button = gtk_button_new_with_label (MARK_COMPLETED);


    gtk_box_pack_start (GTK_BOX (button_box), add_button, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_box), edit_search_button,TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_box), look_ahead_or_back_button, TRUE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (button_box), snooze_button, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_box), mark_completed_button, TRUE, FALSE, 0);
    
    /* These "enter-notify-event" handlers are necessary for the UI to be
     * smooth. */ 
    g_signal_connect (G_OBJECT (mark_completed_button), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    g_signal_connect (G_OBJECT (mark_completed_button), "enter-notify-event",
            G_CALLBACK (mouse_over), NULL);

    /* Add callbacks for the buttons */
    g_signal_connect (G_OBJECT (mark_completed_button), "clicked",
            G_CALLBACK (act_on_selected), tmodel);
    
    g_signal_connect (G_OBJECT (snooze_button), "clicked",
            G_CALLBACK (act_on_selected), tmodel);


    g_signal_connect (G_OBJECT (look_ahead_or_back_button), "clicked",
            G_CALLBACK (set_look_select), NULL);

    g_signal_connect (G_OBJECT (edit_search_button), "clicked",
            G_CALLBACK (set_edit_select_view), NULL);

    gtk_container_add (GTK_CONTAINER (box), button_box);

    return box;
}

/* 
 * FUNC set_main
 *   Removes the current child of the toplevel window.
 * Calls make_main_view_box, loads the new box holding the main_view
 * to the toplevel window
 */
void set_main (GtkWidget *widget)
{
    GtkWidget *window;
    window = gtk_widget_get_toplevel (widget);
    GtkWidget *child;
    child = gtk_bin_get_child (GTK_BIN (window));
    if (child != NULL) {
        gtk_widget_destroy (child);
    }

    GtkWidget *box = make_main_view_box ();

    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (box));

    gtk_widget_show_all (window);
}
