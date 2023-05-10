#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#define DB "db_routine"

void print_code(int rc)
{
    const char *err_msg = sqlite3_errstr(rc);
    printf("%s\n", err_msg);
}

int main(void)
{
    char *errMessage;
    int rc;
    sqlite3 *db;
    sqlite3_stmt *res;
    sqlite3_open_v2(DB, &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create database\nexiting...\n");
        exit (EXIT_FAILURE);
    }

    rc = sqlite3_exec(db, "CREATE TABLE upcoming (description text, date text, category text)", NULL, NULL, &errMessage);
    const char * message = sqlite3_errstr(rc);
    printf("%s\n", message);


    rc = sqlite3_prepare(db, "create table history (description text, date text, category text)", -1, &res, 0);
    print_code(rc);

    rc = sqlite3_step(res);
    print_code(rc);

    sqlite3_prepare(db, "create table attributes (description text, category text, freq int, freq_type text, track_history int)", -1, &res, 0);
    print_code(rc);

    rc = sqlite3_step(res);
    print_code(rc);

    /* New table for notes */
    rc = sqlite3_prepare(db, "create table notes (description text, note text)", -1, &res, 0);
    print_code(rc);


    rc = sqlite3_step(res);
    print_code(rc);


    sqlite3_finalize (res);
    sqlite3_close (db);

    return 0;

}


