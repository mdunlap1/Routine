/*******************************************************************************
 * dates.h
 * Header file for dates.c to share functions with other parts of the program
 *
 ******************************************************************************/

int parse_and_validate_user_date_str (char *date_str, 
                                      int  *month, int  *day, int *year);
/* NOTE: USES MALLOC NEEDS TO BE FREED! */
char * get_current_date_str_in_user_frmt (void);

/* NOTE : USES MALLOC NEEDS TO BE FREED! */
char *make_date_str_sql_frmt(int m, int d, int y);

/* NOTE: USES MALLOC NEEDS TO BE FREED! */
char *convert_date_str_from_sql_to_user_frmt (const char *sql_date_str);

char *make_date_str_sql_frmt(int m, int d, int y);
