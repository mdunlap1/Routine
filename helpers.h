/* used for displaying messages to the user */
void error_dialog (GtkWidget *widget, char *message);
void success_dialog (GtkWidget *widget, char *message);

/* ALLOCATES memory needs to be freed 
 * extract text from textbuffer */
gchar *get_text_from_buffer (GtkTextBuffer *buff);

/* walks through and gets the selected from the model */
gboolean
collect_selected (GtkTreeModel *model,
              GtkTreePath  *path,
              GtkTreeIter  *iter,
              GList       **rowref_list);

 /* 
  * FUNC mouse_over
  *   Used to nudge the window so as to prompt the CellRenderer that editing is 
 * done, prior to the user clicking a button such as mark completed.
 *
 * This makes the UI intuitive in a way that it would otherwise not be 
 * IT IS EXTREMELY IMPORTANT TO USE THIS for all treeviews where the user
 * makes a selection via a checkbox, then edits data in a cell, then
 * clicks a button to submit the changes 
 */
void mouse_over (GtkWidget *widget);

void
cell_edited (GtkCellRendererText *cell,
             const gchar         *path_string,
             const gchar         *new_text,
             gpointer             data);

gboolean check_for_apostrophes (const char *desc);

void
checkbox_select_toggled (GtkCellRendererToggle *cell,
               gchar                 *path_str,
               gpointer               data);

