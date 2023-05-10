/*******************************************************************************
 * helpers.c
 * Utility fuctions for the program that are used across several files. 
 *
 ******************************************************************************/
#include <gtk/gtk.h>

#include "main_enum.h"
#include "setup.h"

/*
 * FUNC error_dialog
 *   Displays an error message as a popup to the user
 */
void error_dialog (GtkWidget *widget, char *message)
{
    GtkWidget *dialog,
              *window;
    window = gtk_widget_get_toplevel (widget);
    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
             message);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/*
 * FUNC success_dialog
 *   Displays a success message to the user after they submit something to 
 *   the db
 *
 *   Note: success_dialog and error_dialog are the same, yet this seemed more
 *         clear as far as ease of reading code was concerned...
 */ 
void success_dialog (GtkWidget *widget, char *message)
{
    GtkWidget *dialog,
              *window;
    window = gtk_widget_get_toplevel (widget);
    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
             message);

    int response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/* 
 * FUNC get_text_from_buffer
 *   Gets text from GtkTextBuffer
 *
 *   Note: Memory : allocates memory needs to be FREED
 */
gchar *get_text_from_buffer (GtkTextBuffer *buff)
{
    GtkTextIter start,
                end;
    gtk_text_buffer_get_bounds (buff, &start, &end);
    gchar *text = gtk_text_buffer_get_slice (buff, &start, &end, FALSE);

    return text;
}

/* 
 * FUNC collect_selected 
 *   Helper function used with gtk_tree_model_foreach
 * Aggregates items that have their checkboxes selected from the main_view 
 *
 * Input:
 *   GtkTreeModel *model : The data of the items to walk through
 *   GtkTreePath *path   : The current GtkTreePath
 *   GtkTreeIter *iter   : The current GtkTreeIter
 *   GList **rowref_list : Where the selected items will be aggregated
 *
 * Return: FALSE (we want to gather all such items up for processing)
 *
 * Note: (copied and modified from GTK tutorial )
 */
gboolean
collect_selected (GtkTreeModel *model,
              GtkTreePath  *path,
              GtkTreeIter  *iter,
              GList       **rowref_list)
{
  guint selected;

  g_assert (rowref_list != NULL);

  gtk_tree_model_get (model, iter, COLUMN_SELECTED, &selected, -1);

  if (selected)
    {
      GtkTreeRowReference  *rowref;

      rowref = gtk_tree_row_reference_new(model, path);

      *rowref_list = g_list_append(*rowref_list, rowref);
    }

  return FALSE; /* do not stop walking the store, call us with next row */
}


/*
 * FUNC mouse_over
 *   Used to trigger enter-notify-event when the mouse goes over buttons so that
 * the text cells in the TreeList will capture the text entered by the user.
 * Without this callback, entering text and clicking submit without first 
 * pressing enter or clicking elsewhere produces non-intuitive and undesireable
 * result, namely that the user's text is NOT submitted correctly 
 */
void mouse_over (GtkWidget *widget)
{
    GtkWidget *window;
    window = gtk_widget_get_toplevel (widget);

    g_signal_emit_by_name (window, "activate-focus");
}

/*
 * FUNC cell_edited
 *   For when text in treeview is edited.
 *
 * Note: Borrowed from gtk3-demo code
 */
void
cell_edited (GtkCellRendererText *cell,        // Renderer for the cell edited
             const gchar         *path_string, // The current path
             const gchar         *new_text,    // For the new text entered
             gpointer             data)        // Model for data
{
    /* Cast data to GtkTreeModel */
    GtkTreeModel *model = (GtkTreeModel *) data;
    /* Get GtkTreePath and GtkTreeIter for model and path */
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (model, &iter, path);

    /* Get column position for text */
    gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

    /* Put in the new text (presumably GTK frees the old text) */
    char *tmp = strdup (new_text);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                        tmp, -1);

    gtk_tree_path_free (path);

    free (tmp);
}

/*
 * FUNC check_for_apostrophes
 *   Used to make sure that descriptions and categories input by user do not
 * contain apostrrophe characters
 */
gboolean check_for_apostrophes (const char *desc) {
    char *p = (char *) desc;
    while (*p != '\0') {
        if (*p == '\'') {
            return TRUE;
        }
        p++;
    }
    return FALSE;
}

/*
 * FUNC select_toggled
 *   toggle the checkbox:
 *
 * Input: GtkCellRenderer *cell : a render to work with
 *        gchar *path_str : a path string to the cell
 *        gpointer data : the relevant data set
 *
 * Note: borrowed from the gtk3-demo code
 */
void
checkbox_select_toggled (GtkCellRendererToggle *cell,
               gchar                 *path_str,
               gpointer               data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreeIter  iter;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    gboolean selcted;

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_SELECTED, &selcted, -1);

    /* do something with the value */
    selcted ^= 1;

    /* set new value */
    gtk_list_store_set (GTK_LIST_STORE (model),
                        &iter,
                        COLUMN_SELECTED,
                        selcted,
                        -1);

    /* clean up */
    gtk_tree_path_free (path);
}

