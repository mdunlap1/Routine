/*******************************************************************************
 * sql_db.h
 * Header for all shared SQL helper functions.
 *
 ******************************************************************************/

#include <sqlite3.h>
#include <gtk/gtk.h>

/* Gets attributes for selected_view.c and ferries them off... */
typedef struct attributes_raw {
    char    *description; 
    char    *due_date; 
    int      freq;
    char    *freq_type;
    char    *category;
    int      track_history;
} Attributes_raw;


/* prototypes */

/* open close and access db */
int init_db ();
int close_db ();
sqlite3 *access_db (); 

/* Grab one attribute */
char *get_last_completion(char *description); /* ALLOCATES MEMORY NEEDS TO BE FREED BY CALLER */
int get_tracking_from_db (char *description);

/* utilities */
int count_rows_of_res(sqlite3 *db, sqlite3_stmt *res);
int count_rows_from_query (char *query);
int desc_already_in_use (const gchar *desc);
char *make_sql_date_modifier(int freq, const char* freq_type); /* ALLOCATES MEMORY NEEDS TO BE FREED BY CALLER */
void log_db_error (int rc);

int add_history(char *desc, char *sql_date_str, char *category);
int update_due_date (char *description, char *sql_date);
int push_back_upcoming (const char *desc, char *sql_date_str);

int add_attributes(const char* desc, const char* category, int freq, const gchar* freq_type, const gchar* track_history);

int add_upcoming(const char *desc, char *sql_date_str, const char *category);

int load_rest_of_attributes_raw_from_desc (Attributes_raw *attributes);

int change_category (char *description, char *new_category);
int change_db_due_date (char *description, char *sql_date, char *category);
int change_frequency (char *description, int freq, const char *freq_type);
int change_hist_date (gchar* description, 
                       gchar* completion_date, 
                       char *new_date);

int remove_entry_from_history (gchar* description, gchar* completion_date);
int remove_from_upcoming (char *description);
int purge_permanently (char *description);


/* Walks GtkListStore and acts on selected */
void change_hist_on_selected (GtkWidget *button, GtkListStore *store);
void snooze_selected_items (GtkWidget *button, GtkListStore *store);
void complete_selected_items (GtkWidget *button, GtkListStore *store);
void remove_selected_historical_entries (GtkWidget *button, GtkListStore *store);

/* Gtk models loaded from db */
GtkTreeModel * create_main_model_from_db (char *query);
GtkTreeModel *create_completion_model_from_db (char* query);

/* end of prototypes */

