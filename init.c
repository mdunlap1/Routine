/*******************************************************************************
 * init.c 
 * Establishes the db connection and starts the program 
 ******************************************************************************/
#include <gtk/gtk.h>
#include "setup.h"
#include "sql_db.h"

/* 
 * FUNC clear_all_and_quit 
 *   Used to destroy the main window and its children.
 *
 * Inputs: *window the toplevel window of the application
 */
static void clear_all_and_quit (GtkWidget *window)
{
    GtkWidget *child;
    child = gtk_bin_get_child (GTK_BIN (window));
    gtk_widget_destroy (child);
    gtk_widget_destroy (GTK_WIDGET (window));
    gtk_main_quit ();
}


/*
 * FUNC main(int argc, char *argv[]
 *   Sets ups and launches the application
 *
 * Inputs command line arguments and count of arguments
 */
int main (int argc, char *argv[])
{

    GtkWidget *window;
    int rc;

    gtk_init (&argc, &argv);

    /* set up the main window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_window_set_title (GTK_WINDOW (window), "Routine Maintenance");
    gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);
    g_signal_connect (G_OBJECT (window), "destroy",
            G_CALLBACK (clear_all_and_quit) , NULL); 

    
    /* Put accelerators to allow ctrl+q and ctrl+w to exit program */
    GtkAccelGroup *accel_group = gtk_accel_group_new ();
    GClosure *quit_func;
    GClosure *window_exit;
    quit_func = g_cclosure_new_object (G_CALLBACK (gtk_main_quit),
                                       G_OBJECT (window));
    window_exit = g_cclosure_new_object (G_CALLBACK (gtk_main_quit),
                                       G_OBJECT (window));

    gtk_accel_group_connect (accel_group,
                             'q',
                             GDK_CONTROL_MASK,
                             GTK_ACCEL_MASK,
                             quit_func);

    gtk_accel_group_connect (accel_group,
                             'w',
                             GDK_CONTROL_MASK,
                             GTK_ACCEL_MASK,
                             window_exit);


    gtk_window_add_accel_group ( GTK_WINDOW (window), accel_group);


    /* Connect to database */ 
    rc = init_db ();

    /* display error if something goes wrong */
    if (rc == SQLITE_CANTOPEN) {
        GtkWidget *fatal_error_label;
        fatal_error_label = gtk_label_new (FATAL_DB_ERROR_NO_ACCESS);
        gtk_container_add (GTK_CONTAINER (window), fatal_error_label);

        gtk_widget_show_all (window);

        gtk_main ();
        exit (1);
    }
    
    /* If database connection good, run main program */
    
    set_main (window); // sets up the main_view

    gtk_main ();

    rc = close_db (); // If db fails to close close_db will log the error

    return 0;
}
