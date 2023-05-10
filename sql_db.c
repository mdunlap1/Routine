/*******************************************************************************
 * sql_db.c
 * Helper functions for accessing the SQL database
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <gtk/gtk.h>

#include "dates.h"
#include "helpers.h"
#include "main_enum.h"
#include "setup.h" 
#include "sql_db.h"

#define DATABASE "db_routine"

static sqlite3 *db = NULL;

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
void complete_selected_items (GtkWidget *button, GtkListStore *store);
void snooze_selected_items (GtkWidget *button, GtkListStore *store);
void change_hist_on_selected (GtkWidget *button, GtkListStore *store);
void remove_selected_historical_entries (GtkWidget *button, GtkListStore *store);

/* Gtk models loaded from db */
GtkTreeModel * create_main_model_from_db (char *query);
GtkTreeModel *create_completion_model_from_db (char* query);

/* end of prototypes */



/* 
 * FUNC init_db
 *   Opens the db connection
 * Returns the sqlite3 status code of the operation
 */
int init_db ()
{
    int rc = sqlite3_open_v2(DATABASE, &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        log_db_error(rc);
    }
    return rc;
}

/* 
 * FUNC close_db
 *   Closes the db connection
 * Returns the sqlite3 status code of the operation
 */
int close_db ()
{
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
        log_db_error(rc);
    }
    return rc;
}

/* 
 * FUNC access_db
 *   Provides access to database
 * Defunct, not currently in use
 */
sqlite3 *access_db ()
{
    return db;
}

/* 
 * FUNC make_sql_date_modifier
 *   Helper function to update_due_date
 * Converts int freq and const char *freq_type to something like: "+3 days" 
 * Returns NULL if memory allocation fails
 *
 * ALLOCATES MEMORY NEEDS TO BE FREED BY CALLER
 */
char * make_sql_date_modifier(int freq, const char* freq_type)
{
    /* calculate number of days if using "weeks" as freq_type */
    int actual_freq;
    const char *actual_type;
    if (strcmp (freq_type, "weeks") == 0) {
        actual_freq = 7 * freq;
        actual_type = "days";
    }
    else {
        actual_freq = freq;
        actual_type = freq_type;
    }

    int num_digits = 0;
    int k = actual_freq;
    while (k != 0) {
        k /= 10;
        num_digits++;
    }
    /* the 3 extra bytes are for ' ', '+' and '\0' in the parameter */
    int num_bytes = 3 + strlen(actual_type) + num_digits;
    char *interval = malloc (sizeof(char) * num_bytes);
    if (interval != NULL) {
        sprintf(interval, "+%d %s", actual_freq, actual_type);
        return interval;
    }
    else
        return NULL;
}

/* 
 * FUNC desc_already_in_use
 *   Check if description is already in use
 * Returns 1 if it is, 0 if not, -1 if there was a db error
 */
int desc_already_in_use (const gchar *desc)
{
    int rc;
    sqlite3_stmt *res;

    char *query = "SELECT * from upcoming where UPPER(description) = UPPER(@desc)";
    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    int idx = sqlite3_bind_parameter_index(res, "@desc");
    rc = sqlite3_bind_text(res, idx, desc, strlen(desc), SQLITE_STATIC); 
    if (rc != SQLITE_OK) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    int count = count_rows_of_res (db, res);

    sqlite3_finalize(res);

    if (count > 0) 
        return 1;
    else if (count == 0)
        return 0;
    else
        return -1;

}

/* 
 * FUNC add_attributes
 *   Adds attributes of item to db
 * Returns 1 on success and -1 on error
 */
int add_attributes(const char* desc, const char* category, int freq, const gchar* freq_type, const gchar* track_history)
{
    sqlite3_stmt *res;
    int rc;
    char* query = "INSERT INTO ATTRIBUTES VALUES "\
                  "( ?, ?, ?, ?, ? )";
                   
    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    int track = ( strcmp(track_history,"y") == 0 ) ? 1 : 0;

    sqlite3_bind_text(res, 1, desc, strlen(desc), SQLITE_STATIC);
    sqlite3_bind_text(res, 2, category, strlen(category), SQLITE_STATIC);
    sqlite3_bind_int(res, 3, freq);
    sqlite3_bind_text(res, 4, freq_type, strlen(freq_type), SQLITE_STATIC);
    sqlite3_bind_int(res, 5, track);

    rc = sqlite3_step(res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize(res);

    return 1;
}

/*
 * FUNC add_history
 *   Adds completion history for item matching desc to the db
 * Returns 1 upon success and -1 upon error
 */
int add_history(char *desc, char *sql_date_str, char *category)
{
    sqlite3_stmt *res;
    int rc;

    char* query = "INSERT INTO HISTORY VALUES "\
                   "( ? , ?, ? )";

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    sqlite3_bind_text(res, 1, desc, strlen(desc), SQLITE_STATIC);
    sqlite3_bind_text(res, 2, sql_date_str, strlen(sql_date_str), SQLITE_STATIC);
    sqlite3_bind_text(res, 3, category, strlen(category), SQLITE_STATIC);

    rc = sqlite3_step(res);
    
    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        return -1;
    }

    sqlite3_finalize(res);

    return 1;
}

/*
 * FUNC push_back_upcoming
 *   Changes the upcoming due date for the item matching desc in the db
 * Returns 1 on success, -1 on error
 */
int push_back_upcoming (const char *desc, char *sql_date_str)
{
    int rc;
    sqlite3_stmt *res;
    char * query = "UPDATE upcoming SET date = ? where description = ?";

    rc = sqlite3_prepare_v2(db, query, -1, &res, NULL);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1; 
    }

    sqlite3_bind_text(res, 1, sql_date_str, strlen(sql_date_str), SQLITE_STATIC);
    rc = sqlite3_bind_text(res,2,desc,strlen(desc), SQLITE_STATIC);

    rc = sqlite3_step(res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize(res);

    return 1;
}

/*
 * FUNC add_upcoming
 *   Adds entry to upcoming due dates in db
 * Returns 1 on success -1 on error
 */
int add_upcoming(const char *desc, char *sql_date_str, const char *category)
{
    sqlite3_stmt *res;
    int rc;

    char* query = "INSERT INTO UPCOMING VALUES "\
                   "( ? , ?, ? )";

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    sqlite3_bind_text(res, 1, desc, strlen(desc), SQLITE_STATIC);
    sqlite3_bind_text(res, 2, sql_date_str, strlen(sql_date_str), SQLITE_STATIC);
    sqlite3_bind_text(res, 3, category, strlen(category), SQLITE_STATIC);

    rc = sqlite3_step(res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize(res);

    return 1;
}


/* 
 * FUNC count_rows_of_res
 *   Counts the number of rows returned by a statement query; 
 * Resets sqlite3_stmt for re-use
 * Returns -1 upon db error
 */
int count_rows_of_res(sqlite3 *db, sqlite3_stmt *res)
{
    int rc;
    const char *tail;
    int count = 0;

    rc = sqlite3_step(res);

    while (rc == SQLITE_ROW) {
        count++;
        rc = sqlite3_step(res);
    }

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_reset(res);
        return -1;
    }

    /* reset the statement for reuse */
    sqlite3_reset(res);

    return count;
}

/* 
 * FUNC count_rows_from_query
 *   Counts the number of rows that a query returns
 * Uses the helper function count_rows_of_res
 * Returns -1 if there is a db error.
 */
int count_rows_from_query (char *query)
{
    int rc;
    sqlite3_stmt *res;
    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }
    int count = count_rows_of_res (db, res);
    sqlite3_finalize (res);
    return count;
}


/*
 * FUNC update_due_date
 *   Updates the due date in the db based on the frequency attributes
 * for the item
 *
 * Returns 1 on success and -1 on error
 */
int update_due_date (char *description, char *sql_date)
{

    int rc;
    sqlite3_stmt *res;
    const char * freq_type;
    int freq;
 
    /* extract freq and freq_type from db */
    char * query = "select freq,freq_type from attributes where description = ?";
    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    rc = sqlite3_bind_text(res, 1, description, strlen(description), SQLITE_TRANSIENT);

    rc = sqlite3_step (res);

    if (rc != SQLITE_ROW) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    freq      = sqlite3_column_int(res, 0);
    freq_type = sqlite3_column_text(res, 1);


    int is_tracked = get_tracking_from_db (description);
    char *last_completed = get_last_completion (description);

    if (is_tracked < 0) {
        fprintf(stderr, DATABASE_FAILED_TO_GET_TRACKING);
        return -1;
    }
    if (last_completed == NULL) {
        fprintf(stderr, DATABASE_FAILED_TO_GET_LAST_COMPLETED);
        return -1;
    }
     

    /* if no repeat remove from upcoming */
    if (strcmp(freq_type, "no_repeat") == 0) {
        sqlite3_stmt *res2;
        char * delete_query = "delete from upcoming where description = ?";
        rc = sqlite3_prepare_v2 (db, delete_query, -1, &res2, 0);
        if (rc != SQLITE_OK) {
            log_db_error(rc);
            return -1;
        }
        sqlite3_bind_text(res2,1,description,strlen(description), SQLITE_TRANSIENT);
        rc = sqlite3_step(res2);

        if (rc != SQLITE_DONE) {
            log_db_error(rc);
            sqlite3_finalize(res2);
            return -1;
        }
        sqlite3_finalize(res2);
    }

    /* if sql_date is after last_completed OR if there is no tracking, update upcoming */
    else if ( ((last_completed == "") || (strcmp(last_completed , sql_date) <= 0) ) ||
           (is_tracked == 0) ) {

        printf("Description: %s\n", description);
        printf("Last completion: %s\n", last_completed);
        printf("New date: %s\n", sql_date);
        printf("strcmp(last_completed , sql_date) = %d\n", strcmp(last_completed , sql_date));

        sqlite3_stmt *res2;

        char * update_query = "UPDATE upcoming SET date = DATE(?, ?) WHERE description = ?";
        
        rc = sqlite3_prepare_v2 (db, update_query, -1, &res2, 0);
        if (rc != SQLITE_OK) {
            log_db_error(rc);
            return -1;
        }
        /* get parameters for update */
        char *interval = make_sql_date_modifier (freq, freq_type); 
        if (interval == NULL) {
            fprintf (stderr, MEM_FAIL_IN "sql_db.c 1\n");
            exit (EXIT_FAILURE);
        }
        sqlite3_bind_text(res2,1, sql_date, strlen(sql_date), SQLITE_TRANSIENT);
        sqlite3_bind_text(res2,2, interval, strlen(interval), SQLITE_TRANSIENT);
        sqlite3_bind_text(res2,3,description,strlen(description), SQLITE_TRANSIENT);
        rc = sqlite3_step(res2);
        if (rc != SQLITE_DONE) {
            log_db_error(rc);
            sqlite3_finalize (res2);
            return -1;
        }
        sqlite3_finalize (res2);
        free (interval);
    }


    if (last_completed && last_completed != "")
        free (last_completed);


    sqlite3_finalize(res);

    return 1;
}

/* 
 * FUNC get_tracking_from_db
 *   Gets the tracking status of the item matching description in the db
 */
int get_tracking_from_db (char *description)
{
    int is_tracked;
    int rc;
    sqlite3_stmt *res;
    char *query = "SELECT track_history FROM attributes\
                   WHERE description = ?"; 

    rc = sqlite3_prepare_v2 (db, query, -1 , &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    rc = sqlite3_bind_text (res, 1, description, strlen(description), 
            SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }


    rc = sqlite3_step (res);
    
    if (rc != SQLITE_ROW) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    is_tracked = sqlite3_column_int (res, 0);

    sqlite3_finalize (res);

    return is_tracked;
}

/*
 * FUNC create_main_model_from_db
 *   Creates the data mode for the main view
 */
GtkTreeModel * create_main_model_from_db (char *query)
{
    int count;
    int status_code;

    const char *tail;
    sqlite3_stmt *res;

    const char *description;
    const char *cat;
    char *date_db_user_frmt;
    const char *date_db_sql_frmt;

    status_code = sqlite3_prepare(db, query, -1, &res, 0);

    if (status_code != SQLITE_OK) {
        log_db_error(status_code);
        return NULL;
    }

    count = count_rows_of_res(db, res); 
    if (count < 0 ) {
        fprintf(stderr, DATABASE_ERROR_FATAL); 
        return NULL;
    }

        
    GtkListStore *store;
    GtkTreeIter iter;

    // create list store 
    store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_BOOLEAN,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);


    // MAKE CALL TO GET DATE STR
    char * date_str = get_current_date_str_in_user_frmt ();
    if (date_str == NULL) {
        fprintf (stderr, MEM_FAIL_IN "sql_db.c 2\n");
        return NULL;
    }
    
    status_code = sqlite3_step(res);

    for (int i = 0; i < count; i++)
    {
      description      = sqlite3_column_text(res,0);
      date_db_sql_frmt = sqlite3_column_text(res,1);
      cat              = sqlite3_column_text(res,2);

      date_db_user_frmt = convert_date_str_from_sql_to_user_frmt (date_db_sql_frmt);
      if (date_db_user_frmt == NULL) {
          free(date_str);
          fprintf(stderr, MEM_FAIL_IN "sql_db.c 3\n");
          return NULL;
      }

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                         COLUMN_SELECTED,            FALSE,
                         COLUMN_DESCRIPTION,         description,
                         COLUMN_DATE_ENTRY, date_str,
                         COLUMN_CATEGORY,            cat,
                         COLUMN_DATE_UNEDITABLE,                date_db_user_frmt,
                         -1);
      free (date_db_user_frmt);
      status_code = sqlite3_step(res);


    }

    sqlite3_finalize(res);
    /* We need date_str in the loop so we do not free it until after */
    free(date_str);

    return GTK_TREE_MODEL (store);
}

/*
 * FUNC change_hist_date
 *   Edits the completion date of an item.
 * Replaces completion_date with new_date in history table of db
 *
 * Returns 1 on success, -1 on error
 */
int change_hist_date (gchar* description, 
                       gchar* completion_date, 
                       char *new_date) 
{
    char *query = "UPDATE history SET date = ? WHERE description = ? AND "\
                  "date = ?"; 

    int rc;
    sqlite3_stmt *res;

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    sqlite3_bind_text (res, 1, new_date, strlen(new_date), SQLITE_TRANSIENT);

    sqlite3_bind_text (res, 2, description, strlen(description), SQLITE_TRANSIENT);
    sqlite3_bind_text (res, 3, completion_date, strlen(completion_date), SQLITE_TRANSIENT);


    rc = sqlite3_step (res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize (res);

    return 1;
}

/*
 * FUNC remove_entry_from_history
 *   Removes the item matching from history table of db
 * Item removed matches description and completion_date
 *
 * Returns 1 on success, -1 on error
 */
int remove_entry_from_history (gchar* description, gchar* completion_date) 
{
    char *query = "DELETE FROM history WHERE description = ? AND "\
                  "date = ?"; 

    int rc;
    sqlite3_stmt *res;

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    sqlite3_bind_text (res, 1, description, strlen(description), SQLITE_TRANSIENT);
    sqlite3_bind_text (res, 2, completion_date, strlen(completion_date), SQLITE_TRANSIENT);

    rc = sqlite3_step (res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize (res);

    return 1;
}
    
 
/*
 * FUNC load_rest_of_attributes_raw_from_desc
 *   Gets attributes information from db and loads them into an Attributes_raw
 * struct provided by the caller. 
 *
 * Returns 1 on success -1 on error 
 *
 * ASSERTION: Assumes that the Attributes_raw struct has already had the 
 * description added to it. 
 */
int load_rest_of_attributes_raw_from_desc (Attributes_raw *attributes)
{
    const char *desc = attributes->description;
    int rc;
    sqlite3_stmt *res;
    char *query1 = "SELECT * FROM attributes WHERE description = ?";
    rc = sqlite3_prepare_v2 (db, query1, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }
    sqlite3_bind_text (res, 1, desc, strlen(desc), SQLITE_STATIC);

    rc = sqlite3_step(res);
    if (rc != SQLITE_ROW) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    const char *category  = sqlite3_column_text (res, 1);
    const char *freq_type = sqlite3_column_text (res, 3);

    int freq = sqlite3_column_int (res, 2);
    int is_tracked = sqlite3_column_int (res, 4);

    attributes->category = strdup (category);
    attributes->freq_type = strdup (freq_type);

    if (attributes->category == NULL || attributes->freq_type == NULL) {
        fprintf (stderr, MEM_FAIL_IN "sql_db.c 4\n");
        return -1;
    }

    attributes->freq = freq;
    attributes->track_history = is_tracked;

    sqlite3_finalize (res);

    sqlite3_stmt *res2;
    char *query2 = "SELECT date FROM upcoming WHERE description = ?";
    rc = sqlite3_prepare_v2 (db, query2, -1, &res2, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }
    sqlite3_bind_text (res2, 1, desc, strlen(desc), SQLITE_STATIC);

    rc = sqlite3_step (res2);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        log_db_error(rc);
        return -1;
    }

    const char *due;
    due = sqlite3_column_text (res2, 0);
    
    if (due != NULL)
        attributes->due_date = strdup(due);
    else
        attributes->due_date = strdup(NOT_DUE);

    if (attributes->due_date == NULL) {
        free (attributes->category);
        free (attributes->freq_type);
        fprintf (stderr, MEM_FAIL_IN "sql_db.c 5\n");
        return -1;
    }

    sqlite3_finalize (res2);

    return 1;
}

/* 
 * FUNC create_completion_model_from_db
 *   Creates a completion model from the db using query
 * Returns NULL on error
 *
 * Note: Using NULL for a completion in GTK does not cause a fatal error
 *       therefore the program does not crash out when this happens 
 */
GtkTreeModel *
create_completion_model_from_db (char* query)
{
    GtkListStore *store;
    GtkTreeIter iter;

    int rc;
    int count;

    sqlite3_stmt *res;
    const char *string;

    rc = sqlite3_prepare(db, query, -1,
            &res, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, COMPLETION_ERROR);
        log_db_error(rc);
        return NULL;
    }

    store = gtk_list_store_new (1, G_TYPE_STRING);

    rc = sqlite3_step(res);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        fprintf(stderr, COMPLETION_ERROR);
        log_db_error(rc);
        return NULL;
    }

    while (rc == SQLITE_ROW) {
        string = sqlite3_column_text(res,0);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, string, -1);
        rc = sqlite3_step(res);
    }

    sqlite3_finalize(res);

    return GTK_TREE_MODEL (store);

}

/* 
 * FUNC change_db_due_date
 *   Changes the due date of an item in the db
 * Returns 1 on success, -1 on error
 */
int change_db_due_date (char *description, char *sql_date, char *category)
{
    int rc;

    sqlite3_stmt *res0;
    char *query_if_in_upcoming = "SELECT * FROM UPCOMING WHERE description = ?";
    rc = sqlite3_prepare_v2 (db, query_if_in_upcoming, -1, &res0, 0);
    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }
    sqlite3_bind_text (res0, 1, description, strlen(description), SQLITE_TRANSIENT);

    int is_in_upcoming = count_rows_of_res (db, res0);
    if (is_in_upcoming < 0) {
        fprintf(stderr, DATABASE_FAIL_TO_CHANGE_DUE_DATE);
        sqlite3_finalize(res0);
        return -1;
    }

    sqlite3_finalize (res0);

    if (is_in_upcoming == 1) {

        sqlite3_stmt *res;

        char *query = "UPDATE upcoming SET date = ? WHERE description = ?";

        rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

        if (rc != SQLITE_OK) {
            log_db_error(rc);
            return -1;
        }

        sqlite3_bind_text (res, 1, sql_date, strlen(sql_date), SQLITE_STATIC);
        sqlite3_bind_text (res, 2, description, strlen(description), SQLITE_STATIC);

        rc = sqlite3_step (res);

        if (rc != SQLITE_DONE) {
            log_db_error(rc);
            sqlite3_finalize(res);
            return -1;
        }

        sqlite3_finalize (res);
    }
    /* If no due date, give it a new due date entry */
    else {
        int add_success = add_upcoming (description, sql_date, category);
        if (add_success < 0) {
            fprintf(stderr, DATABASE_FAIL_TO_CHANGE_DUE_DATE);
            return -1;
        }
    }
}

/*
 * FUNC change_frequency
 *   Changes the frequency used to calculate due dates for an item
 * Returns 1 on success and -1 on error
 */
int change_frequency (char *description, int freq, const char *freq_type)
{
    int rc;
    sqlite3_stmt *res;

    char *query = "UPDATE attributes SET freq = ?, freq_type = ? WHERE "\
                   "description = ?";

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    rc = sqlite3_bind_int (res, 1, freq); 
    rc = sqlite3_bind_text (res, 2, freq_type, strlen(freq_type), SQLITE_TRANSIENT);
    rc = sqlite3_bind_text (res, 3, description , strlen(description), SQLITE_TRANSIENT);

    rc = sqlite3_step (res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize (res);

    return 1;
}

/*
 * FUNC change_category
 *   Changes the category of the item matching description to new_category 
 * Returns 1 on success, -1 on error
 */
int change_category (char *description, char *new_category)
{
    char *queries[]={"UPDATE history SET category = ? WHERE description = ?",
                     "UPDATE attributes SET category = ? WHERE description = ?",
                     "UPDATE upcoming SET category = ? WHERE description = ?"
                    };

    int num_queries = 3;
    int rc;
    for (int i = 0; i < num_queries; i++)
    {
        sqlite3_stmt *res;
        rc = sqlite3_prepare_v2 (db, queries[i], -1, &res, 0);
        if (rc != SQLITE_OK) {
            log_db_error(rc);
            return -1;
        }

        sqlite3_bind_text (res, 1, new_category, strlen(new_category), SQLITE_TRANSIENT);
        sqlite3_bind_text (res, 2, description, strlen(description), SQLITE_TRANSIENT);

        rc = sqlite3_step (res);
        if (rc != SQLITE_DONE) {
            log_db_error(rc);
            return -1;
        }

        sqlite3_finalize (res);

    }

    return 1;

}

/*
 * FUNC remove_from_upcoming
 *   Removes the item matching description from the upcoming table in the db
 * Returns 1 on success, -1 on error
 */
int remove_from_upcoming (char *description)
{
    int rc;
    sqlite3_stmt *res;

    char *query = "DELETE FROM upcoming WHERE description = ?";

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return -1;
    }

    sqlite3_bind_text (res, 1, description, strlen(description), SQLITE_TRANSIENT);
    rc = sqlite3_step (res);

    if (rc != SQLITE_DONE) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return -1;
    }

    sqlite3_finalize (res);

    return 1;
}

/*
 * FUNC purge_permanently
 *   Removes the item matching description from the db
 */
int purge_permanently (char *description)
{
    char *queries[] = {"DELETE FROM upcoming WHERE description = ?",
                       "DELETE FROM history WHERE description = ?",
                       "DELETE FROM attributes WHERE description = ?"
    };

    int num_queries = 3;
    int rc;
    for (int i = 0; i < num_queries; i++)
    {
        sqlite3_stmt *res;
        rc = sqlite3_prepare_v2 (db, queries[i], -1, &res, 0);
        
        if (rc != SQLITE_OK) {
            log_db_error(rc);
            return -1;
        }

        sqlite3_bind_text (res, 1, description, strlen(description), SQLITE_TRANSIENT);

        rc = sqlite3_step (res);

        if (rc != SQLITE_DONE) {
            log_db_error(rc);
            sqlite3_finalize(res);
            return 1;
        }

        sqlite3_finalize (res);

    }
    return 1;
}

/*
 * FUNC remove_selected_historical_entries
 *   Walks through a GtkListStore of the completion data for an item, aggregates
 * selected items. Then removes the corresponding entries from the db
 */
void remove_selected_historical_entries (GtkWidget *button, GtkListStore *store)
{
  GList *rr_list = NULL;    /* list of GtkTreeRowReferences to remove */
  GList *node;
  gchar *description;
  gchar *date_completed; 

  /* Gather up selected items */
  gtk_tree_model_foreach (GTK_TREE_MODEL(store),
                          (GtkTreeModelForeachFunc) collect_selected,
                          &rr_list);

  /* Walk through list of selected items and process them */
  for (node = rr_list;  node != NULL;  node = node->next) {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)node->data);

      if (path) {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
              gtk_tree_model_get (GTK_TREE_MODEL (store),
                                  &iter,
                                  COLUMN_DESCRIPTION, &description,
                                  COLUMN_DATE_UNEDITABLE , &date_completed,
                                  -1);

              /* The date should be valid so we will not check stat */
              int stat,m,d,y;
              stat = parse_and_validate_user_date_str 
                  (date_completed, &m, &d, &y);

              /* remove entry from history table */
              char *completion_date = make_date_str_sql_frmt (m,d,y);
              if (completion_date == NULL) {
                  fprintf (stderr, MEM_FAIL_IN "sql_db.c 6\n");
                  exit (EXIT_FAILURE);
              }

              int h_stat = 1;
              h_stat = remove_entry_from_history (description, completion_date);
              if (h_stat < 0)
                  error_dialog (button, DATABASE_FAIL_REMOVE_HIST);
              else 
                  gtk_list_store_remove(store, &iter);

              free (completion_date);
              free (description);
              free (date_completed);
          }
          gtk_tree_path_free(path);
      }
  }
  g_list_free_full(rr_list, (GDestroyNotify)gtk_tree_row_reference_free);
}


/*
 * FUNC change_hist_on_selected
 *   Walks through the GtkListStore containing the completion histroy of an item
 * and modifes the completion date based on user input of selected items.
 */
void change_hist_on_selected (GtkWidget *button, GtkListStore *store)
{
  GList *rr_list = NULL;    /* list of GtkTreeRowReferences to remove */
  GList *node;
  gchar *description;
  gchar *old_date; 
  gchar *new_date;

  /* Gather up selected items */
  gtk_tree_model_foreach (GTK_TREE_MODEL(store),
                          (GtkTreeModelForeachFunc) collect_selected,
                          &rr_list);

  /* Walk through list of selected items and process them */
  for (node = rr_list;  node != NULL;  node = node->next) {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)node->data);

      if (path) {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
              gtk_tree_model_get (GTK_TREE_MODEL (store),
                                  &iter,
                                  COLUMN_DESCRIPTION, &description,
                                  COLUMN_DATE_ENTRY, &new_date,
                                  COLUMN_DATE_UNEDITABLE , &old_date,
                                  -1);

              /* Check user input */
              int stat,m,d,y;
              stat = parse_and_validate_user_date_str (
                      (char*) new_date, &m, &d, &y);

              if ( stat == 0 ) {
                  error_dialog (button, INVALID_DATE\
                                         DATE_FRMT_EXPLAIN);

                  free (description);
                  free (new_date);
                  free (old_date);
                  gtk_tree_path_free(path);

                  continue;
              }

              /* Convert user input new_date to SQL format */
              char *new_date_sql = make_date_str_sql_frmt (m, d, y);
              if (new_date_sql == NULL) {
                  fprintf (stderr, MEM_FAIL_IN "sql_db.c 7\n");
                  exit (EXIT_FAILURE);
              }

              /* Convert old_date to SQL format */
              int m2, d2, y2;
              stat = parse_and_validate_user_date_str (old_date, 
                      &m2, &d2, &y2);
              char *old_date_sql = make_date_str_sql_frmt (m2, d2, y2);
              if (old_date_sql == NULL) {
                  fprintf (stderr, MEM_FAIL_IN "sql_db.c 8\n");
                  exit (EXIT_FAILURE);
              }

              /* Change histroical entry */
              int change_status = 1;
              change_status = change_hist_date (description, old_date_sql, new_date_sql);
              if (change_status < 0) {
                  error_dialog (button, DATABASE_EDIT_HIST_FAIL);

                  free (new_date_sql);
                  free (old_date_sql);

                  free (description);
                  free (new_date);
                  free (old_date);

                  gtk_tree_path_free(path);

                  continue;
              }

              gtk_list_store_set (store, &iter, 
                                  COLUMN_DATE_UNEDITABLE, new_date, 
                                  -1);

              free (new_date_sql);
              free (old_date_sql);
              free (description);
              free (new_date);
              free (old_date);

          }
          gtk_tree_path_free(path);
      }
  }
  g_list_free_full(rr_list, (GDestroyNotify)gtk_tree_row_reference_free);
}

/*
 * FUNC snooze_selected_items 
 *   Walks through a GtkListStore of items, pushes the due dates of items 
 * selected by user back to the date speicified by the user.
 */
void snooze_selected_items (GtkWidget *button, GtkListStore *store)
{
  GList *rr_list = NULL;    /* list of GtkTreeRowReferences to remove */
  GList *node;

  gchar *description;
  gchar *date;

  /* Gather up selected items */
  gtk_tree_model_foreach (GTK_TREE_MODEL(store),
                          (GtkTreeModelForeachFunc) collect_selected,
                          &rr_list);

  /* Walk through list of selected items and process them */
  for (node = rr_list;  node != NULL;  node = node->next) {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)node->data);

      if (path) {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
              gtk_tree_model_get (GTK_TREE_MODEL (store),
                                  &iter,
                                  COLUMN_DESCRIPTION, &description,
                                  COLUMN_DATE_ENTRY, &date,
                                  -1);

              /* Check user input */
              int stat,m,d,y;
              stat = parse_and_validate_user_date_str (
                      (char*) date, &m, &d, &y);

              if ( stat == 0 ) {
                  error_dialog (button, INVALID_DATE\
                                         DATE_FRMT_EXPLAIN);
                  free (date);
                  free (description);
                  gtk_tree_path_free(path);

                  continue;
              }

              /* Convert user input new_date to SQL format */
              char *date_sql_frmt = make_date_str_sql_frmt (m, d, y);
              if (date_sql_frmt == NULL) {
                  fprintf (stderr, MEM_FAIL_IN "sql_db.c 9\n");
                  exit (EXIT_FAILURE);
              }
              
              /* Snooze the item */
              int pb_success = 1;
              pb_success = push_back_upcoming (description, date_sql_frmt);
              if (pb_success < 0)
                  error_dialog (button, DATABASE_SNOOZE_FAIL);
              else
                  gtk_list_store_remove (store, &iter);
                                  
              free (date);
              free (description);
              free (date_sql_frmt);

          }
          gtk_tree_path_free(path);
      }
  }
  g_list_free_full(rr_list, (GDestroyNotify)gtk_tree_row_reference_free);
}


/*
 * FUNC complete_selected_items
 *   Walks through a GtkListStore of items, marks the items selected by user
 * with completion dates also provided by the user.
 */
void complete_selected_items (GtkWidget *button, GtkListStore *store)
{
  GList *rr_list = NULL;    /* list of GtkTreeRowReferences to remove */
  GList *node;

  gchar *description;
  gchar *date;
  gchar *category;

  /* Gather up selected items */
  gtk_tree_model_foreach (GTK_TREE_MODEL(store),
                          (GtkTreeModelForeachFunc) collect_selected,
                          &rr_list);

  /* Walk through list of selected items and process them */
  for (node = rr_list;  node != NULL;  node = node->next) {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)node->data);

      if (path) {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
              gtk_tree_model_get (GTK_TREE_MODEL (store),
                                  &iter,
                                  COLUMN_DESCRIPTION, &description,
                                  COLUMN_DATE_ENTRY, &date,
                                  COLUMN_CATEGORY , &category,
                                  -1);

              /* Check user input */
              int stat,m,d,y;
              stat = parse_and_validate_user_date_str (
                      (char*) date, &m, &d, &y);

              if ( stat == 0 ) {
                  error_dialog (button, INVALID_DATE\
                                         DATE_FRMT_EXPLAIN);
                  free (date);
                  free (description);
                  free (category);
                  gtk_tree_path_free(path);

                  continue;
              }

              /* Convert user input new_date to SQL format */
              char *date_sql_frmt = make_date_str_sql_frmt (m, d, y);
              if (date_sql_frmt == NULL) {
                  fprintf (stderr, MEM_FAIL_IN "sql_db.c 10\n");
                  exit (EXIT_FAILURE);
              }

              /* Update completion on item */
              int is_tracked = get_tracking_from_db (description);
              if (is_tracked < 0) {
                  error_dialog (button, DATABASE_FAILED_TO_GET_TRACKING);

                  gtk_tree_path_free (path);
                  free (date_sql_frmt);
                  free (description);
                  free (date);
                  free (category);

                  continue;
              }

              int h_success = 1;
              if (is_tracked == 1)
                  h_success = add_history (description, date_sql_frmt, category);
              if (h_success < 0) {
                  error_dialog (button, DATABASE_MARK_COMPLETE_FAIL);

                  free (date);
                  free (description);
                  free (category);
                  gtk_tree_path_free (path);
                  free (date_sql_frmt);

                  continue;
              }

              int u_success = 1;
              u_success = update_due_date (description, date_sql_frmt);
              if (u_success < 0) {
                  error_dialog (button, DATABASE_UPDATE_DUE_DATE_FAIL);

                  free (date);
                  free (description);
                  free (category);
                  gtk_tree_path_free (path);
                  free (date_sql_frmt);

                  continue;
              }

              free (date_sql_frmt);
              free (date);
              free (description);
              free (category);

          }
          gtk_tree_path_free(path);
      }
  }
  g_list_free_full(rr_list, (GDestroyNotify)gtk_tree_row_reference_free);
}

/*
 * FUNC get_last_completion
 *   Returns the SQL formatted date string of the date an item was last 
 * completed that matches description or returns "" is no such item was 
 * found; returns NULL on error 
 *
 * Helper function to update_due_date, adding a completion record for an item
 * that pre-dates the most recent completion date should NOT change the next 
 * due date for the item.
 *
 * ALLOCATES MEMORY NEEDS TO BE FREED BY CALLER
 */
char *get_last_completion(char *description)
{
    int rc;
    sqlite3_stmt *res;
    char *query = "select MAX(date) from history where description = ?";

    rc = sqlite3_prepare_v2 (db, query, -1, &res, 0);

    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return NULL;
    }

    rc = sqlite3_bind_text (res, 1, description, strlen(description), 
            SQLITE_STATIC);
    
    if (rc != SQLITE_OK) {
        log_db_error(rc);
        return NULL;
    }

    rc = sqlite3_step (res);
    
    if (rc != SQLITE_ROW) {
        log_db_error(rc);
        sqlite3_finalize(res);
        return NULL;
    }

    const char *date = sqlite3_column_text(res , 0);
    if (date != NULL) {
        char *last_completed = strdup(date);
        sqlite3_finalize (res);
        return last_completed;
    }
    else {
        sqlite3_finalize (res);
        return "";
    }

}

void log_db_error (int status_code)
{
    const char *err_msg = sqlite3_errstr(status_code);
    /* Note that we do not need to free err_msg with sqlite3_free */
    fprintf(stderr, "Error: %s\n", err_msg);
}

