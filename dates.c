/*******************************************************************************
 * dates.c
 * Helper functions for date manipulations in program.
 *
 * NOTE: Some functions could probably be replaced with sqlite3.h functions
 * 
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h> /* needed because we make use of "setup.h" */

#include "dates.h"
#include "setup.h"


#define SQL_FRMT_STR "%d-%02d-%02d" /* Note: SQL format is  year, month, day */

static int num_days_in_months_of_non_leap_year[13] = 
{0,31,28,31,30,31,30,31,31,30,31,30,31};


/* prototypes */
static void mdy_to_user_frmt (int seq[]);
static void user_frmt_to_mdy (int seq[]);
static int validate_mdy (int month, int day, int year);

/* interface */
char *make_date_str_sql_frmt(int m, int d, int y);
int parse_and_validate_user_date_str (char* date_str, int *month, int *day, int *year);
char * get_current_date_str_in_user_frmt (void);
char *convert_date_str_from_sql_to_user_frmt (const char *sql_date_str);
/* end prototypes */

/*
 * FUNC user_frmt_to_mdy
 *   Takes an integer array that contains month, day, year (in user specified
 * ordering) and mutates it into an array that is in month, day, year ordering.
 *
 * Depends upon a macro which is a permutation of M,D,Y and to be found in the
 * language header specified in setup.h
 */
static void user_frmt_to_mdy (int seq[])
{

#ifdef MDY
    return;
#endif

    int m,d,y;

#ifdef MYD
    m = seq[0];
    y = seq[1];
    d = seq[2];
    seq[0] = m;
    seq[1] = d;
    seq[2] = y;
    return;
#endif

#ifdef YDM
    y = seq[0];
    d = seq[1];
    m = seq[2];
    seq[0] = m;
    seq[1] = d;
    seq[2] = y;
    return;
#endif

#ifdef YMD
    y = seq[0];
    m = seq[1];
    d = seq[2];
    seq[0] = m;
    seq[1] = d;
    seq[2] = y;
    return;
#endif

#ifdef DYM
    d = seq[0];
    y = seq[1];
    m = seq[2];
    seq[0] = m;
    seq[1] = d;
    seq[2] = y;
    return;
#endif

#ifdef DMY
    d = seq[0];
    m = seq[1];
    y = seq[2];
    seq[0] = m;
    seq[1] = d;
    seq[2] = y;
    return;
#endif
}

/*
 * FUNC mdy_to_user_frmt
 *   Takes an integer array with month, day and year (in that order) and mutates
 * it to be in user specified format.
 *
 * Format is determined by macro such as MDY (or any other permutation of
 * the letters M,D,Y.
 *
 * See language configuration header (selected in setup.h) for configuration
 * ie (text_en.h)
 *
 */
static void mdy_to_user_frmt (int seq[]) 
{
#ifdef MDY
    return;
#endif

    int m,d,y;

#ifdef MYD
    m = seq[0];
    d = seq[1];
    y = seq[2];

    seq[0] = m;
    seq[1] = y;
    seq[2] = d;
    return;
#endif

#ifdef YDM
    m = seq[0];
    d = seq[1];
    y = seq[2];

    seq[0] = y;
    seq[1] = d;
    seq[2] = m;
    return;
#endif

#ifdef YMD
    m = seq[0];
    d = seq[1];
    y = seq[2];

    seq[0] = y;
    seq[1] = m;
    seq[2] = d;
    return;
#endif

#ifdef DYM
    m = seq[0];
    d = seq[1];
    y = seq[2];

    seq[0] = d;
    seq[1] = y;
    seq[2] = m;
    return;
#endif

#ifdef DMY
    m = seq[0];
    d = seq[1];
    y = seq[2];

    seq[0] = d;
    seq[1] = m;
    seq[2] = y;
    return;
#endif
}

/* 
 * FUNC validate_mdy
 *   Parses a string of form mm/dd/yyyy into month, day, year ints.
 * Checks if this is a validate calendar date
 *
 * Returns: 1 if valid, 0 otherwise. 
 */
static int validate_mdy (int month, int day, int year)
{
    if (year <= 0)
        return 0;
    if (month < 1 || month > 12)
        return 0;
    int min, max;
    min = 1;
    max = num_days_in_months_of_non_leap_year[month];
    if (__isleap(year))
        max++;
    if (day < min || day > max)
        return 0;

    return 1;
}


/* 
 * FUNC make_date_str_sql_frmt
 * 
 * Input: a month, day and year (all as ints)
 *
 * Return: char *date_str the SQL formatted date string
 *         or NULL upon error
 *
 * NOTE: USES MALLOC, will have to be FREED 
 */
char *make_date_str_sql_frmt(int m, int d, int y)
{
    char *date_str = malloc(sizeof(char) * 11);
    if (date_str == NULL) {
        return NULL;
    }
    int status = sprintf(date_str, SQL_FRMT_STR, y, m , d);
    /* SQL_FRMT_STR should give dats as yyyy-mm-dd, so at least 10 chars */
    if (status < 10){
        free (date_str);
        return NULL;
    }
    return date_str;
}


/*
 * FUNC parse_and_validate_user_date_str
 *   Checks if the char* date_str is a valid date in mdy format
 * Returns 1 if it is, 0 otherwise.
 *
 * Also stores the month, day, and year of the date in the int* provided by
 * caller 
 */
int parse_and_validate_user_date_str (char* date_str, int *month, int *day, int *year)
{
    /* scan user date string */
    int k1, k2, k3, res, valid;
    res = sscanf(date_str, DATE_FRMT_STR, &k1, &k2, &k3);
    if (res != 3)
        return 0;

    /* convert k1, k2, k3 to mdy format */
    int input[3] = {k1, k2, k3};
    user_frmt_to_mdy(input);
    int m,d,y;
    m = input[0];
    d = input[1];
    y = input[2];

    /* validate the date */
    valid = validate_mdy(m,d,y);
    if (valid) {
        *month = m;
        *day   = d;
        *year  = y;
        return 1;
    }
    else 
        return 0;
}

/* 
 * FUNC get_current_date_str_in_user_frmt
 *   Gets the current date and returns it as a formatted string
 * returns a NULL pointer if it encoutners an error
 *
 * Format is determined by DATE_FRMT_STR where DATE_FRMT_STR displays 
 * and the ordering of MDY is determined by which macro (of the permutations)
 * of MDY is defined...
 *
 * For example if DATE_FRMT_STR is "%d/%d/%d" we will get the current date as:
 * m/d/y
 *
 * NOTE: ALLOCATES MEMORY NEEEDS TO BE FREED
 */
char * get_current_date_str_in_user_frmt (void)
{
    /* get the current date */
    time_t mytime;
    mytime = time(NULL);
    struct tm *current_time;
    current_time = localtime(&mytime);
    int month,day,year;
    month = current_time->tm_mon + 1;     // since months range from [0,11]
    day   = current_time->tm_mday;
    year  = current_time->tm_year + 1900; // Have to add 1900 to get current yr

    char * date_str = NULL;
    date_str = malloc(sizeof(char) * DATE_STR_LIMIT);
    if (date_str == NULL)
        return NULL;

    /* convert mdy to user format */
    int seq[3] = {month, day, year};
    mdy_to_user_frmt(seq);
    int k1, k2, k3;
    k1 = seq[0];
    k2 = seq[1];
    k3 = seq[2];

    int stat = sprintf(date_str, DATE_FRMT_STR, k1, k2, k3);

    return date_str;
} 

/*
 * FUNC convert_date_str_from_sql_to_user_frmt
 *   Takes a SQL formatted date and reverses it to "user format"
 * Will not add preceeding zeros
 * NOTE: USES MALLOC will have to be FREED
 */
char *convert_date_str_from_sql_to_user_frmt (const char *sql_date_str)
{
    int m,d,y;
    sscanf(sql_date_str, SQL_FRMT_STR,&y,&m,&d);
    char *date_str = malloc (sizeof (char) * DATE_STR_LIMIT);
    if (date_str == NULL) {
        return NULL;
    }

    /* convert mdy to user format */
    int seq[3] = {m, d, y};
    mdy_to_user_frmt(seq);
    int k1, k2, k3;
    k1 = seq[0];
    k2 = seq[1];
    k3 = seq[2];

    int res = sprintf(date_str, DATE_FRMT_STR, k1,k2,k3);
    return date_str;
}
