/*******************************************************************************
 * text_en.h
 * Used to store all visible text in program for the english language.
 * Use this file to store all the text that gets displayed so that we can 
 * change the language that the program uses! 
 ******************************************************************************/

/* Specifies how a date is to be displayed to the user.
 * DATE_FRMT_STR string handles date printing (if preceeding zeros are desired 
 * then the installer should choose the DATE_FRMT_STR accordingly.
 *
 * MDY is defined if the DATE_FRMT_STR should render the month, then the day,
 * then the year, for example:
 *
 * January 15 2023 is rendered as 1/15/2023
 *
 * IF DATE_FRMT_STR = "%d/%d/%d" and MDY is defined.
 *
 * IMPORTANT: make sure that exactly one permutation of MDY is defined. 
 *
 * DATE_STR_LIMIT determines how much memory to allocate for the datestring.
 *
 * Representing years with two digts such as; 1/15/23 is NOT currently supported
 */
#define DATE_FRMT_STR "%d/%d/%d"
#define MDY 
#define DATE_STR_LIMIT 11
#define DATE_FRMT_EXPLAIN "m/d/y"

/******* for language specific input limits *******/
// NOTE: these are not currently being used 
#define DESC_LIMIT 101
#define FREQ_TYPE_LIMIT 7
#define CATEGORY_LIMIT 21

/******* For init.c *******/
#define FATAL_DB_ERROR_NO_ACCESS "Fatal error: cannot access database\n"

/******* For main_view.c *******/

#define TODAY_VIEW_HEADING "Today's Items"

#define DESCRIPTION "Description"

#define DUE "Due"

#define COMPLETE_PUSH_BACK  "Completed on /\nPush back to"

#define CATEGORY "Category"

#define NO_ITEMS_CURRENTLY_DUE "There are no items currently due."

#define ADD "add"

#define EDIT_SEARCH "edit / search"

#define LOOK_AHEAD_OR_BACK "look ahead / back"

#define SNOOZE "snooze"

#define MARK_COMPLETED "mark completed"

//#define SEE_DIRECTIONS "see directions"

/******* For add_view.c *******/

// Uses DESCRIPTION

#define DUE_ON "Initially due"

#define FREQUENCY "Frequency"

#define BACK "back"

#define SUBMIT "submit"

// Uses CATEGORY

#define DAYS "days"

#define MONTHS "months"

#define YEARS "years"

#define NO_REPEAT "no repeat"

#define TRACK_HISTORY "Track history?"

#define YES "yes"

#define NO "no"

#define DESCRIPTION_COLLISION "ERROR: Description matches one already in database"

#define CANNOT_HAVE_APOSTROPHES "ERROR: Descriptions and Categories cannot have apostrophes"

#define INVALID_DATE "ERROR: Date format is invalid\nShould be formatted: "

/******** For look_select_view.c *******/

#define VIEW "view"

#define FROM "From"

#define TO "to"

#define INCLUSIVE "inclusive"

#define LOOK "Look"
#define AHEAD "ahead"
#define WEEKS "weeks"
#define DAY "day"
#define WEEK "week"
#define MONTH "month"
// Uses BACK
// Uses MONTHS
// Uses INVALID_DATE

#define START_BEFORE_END "Start date needs to be earlier than or equal to end date"


/******* For use in ahead_back_view.c *******/
#define MAIN "main"
#define NO_HIST_IN_RANGE "No completed items in selected range."
#define REMOVE "remove"
#define CHANGE_DATE "change date"
#define COMPLETED_ON "Completed on"
#define CHANGE_TO "Change completion date to"
#define NO_ITEMS_IN_RANGE "No items to display in selected range" 
#define ITEMS_DUE_IN_RANGE  "Items due in range selected"
#define ITEMS_COMPLETED_IN_RANGE "Items completed in range selected"

/******* For use in edit_select_view *******/
#define SELECT "select"
#define REPEAT_EVERY "Repeat every"
// Uses TRACK_HISTORY


/******* For use in selected_view.c *******/
#define UPDATE_COMPLETED "Update completed"
#define UPDATE_ATTRIBUTES "Update attributes"
// Uses DUE_ON
#define NOT_DUE "no due date"
#define CHANGE_DUE_DATE "change due date"
#define EVERY "Every"
#define CHANGE_FREQ "change frequency"
#define CHANGE_CATEGORY "change category"
#define REMOVE_FROM_UPCOMING "remove from upcoming"
#define REMOVE_AND_DELETE_HIST "Remove item and delete history"
#define PURGE_WARN "Are you sure you want to remove all item data?" 
#define PERMANENTLY_REMOVE "Permanently delete item and history"
#define PURGE_ITEM "purge item"

/******* For database error handling and success *******/
#define FATAL_ERROR "Error: Fatal error encountred."
#define COMPLETION_ERROR "Error: Unable to load completion model."
#define SUCCESS "success!"
#define PURGE_SUCCESS "Item suscessfuly purged from database."
#define DATABASE_ERROR "Database error!\n"
#define DATABASE_ERROR_FATAL "Fatal database error encountered.\n"
#define DATABASE_HIST_FAIL "Error: Failed to update item history."
#define DATABASE_UPDATE_DUE_DATE_FAIL "Error: Failed to update item due date."
#define DATABASE_FAILED_TO_GET_TRACKING "Error: Failed to get tracking for item.\n"
#define DATABASE_FAILED_TO_GET_LAST_COMPLETED "Error: Failed to get date of last completion for item.\n"
#define DATABASE_SNOOZE_FAIL "Error: Failed to snooze item."
#define DATABASE_ADD_FAIL "Error: Failed to add new item."
#define DATABASE_ADD_UPCOMING_FAIL "Error: Failed to add item to upcoming due."
#define DATABASE_CANNOT_LOAD_ATTRIBUTES_FATAL "FATAL Error: Unable to load attributes of item.\n"
#define DATABASE_FAILED_TO_CHANGE_CATEGORY "Error: Failed to change item category.\n Please try again before proceeding\nas the database may be corrupted."
#define DATABASE_FAIL_TO_CHANGE_DUE_DATE "Error: Failed to change due date.\n"
#define DATABASE_FREQ_FAIL "Error: Unable to change frequency."
#define DATABASE_EDIT_HIST_FAIL "Error: Unable to change completion date."
#define DATABASE_FAIL_REMOVE_HIST "Error: Unable to remove completion entry."
#define DATABASE_REMOVE_UPCOMING_FAIL "Error: Unable to remove from upcoming."
#define DATABASE_PURGE_ITEM_FAIL "ERROR: Something went wrong when purging the item\nThe database may be seriously corrupted.\nPlease correct it before proceeding."
#define DATABASE_ATTRIBUTES_MODEL_FAIL "Fatal Error: Unable to load attrubutes model"
#define DATABASE_MARK_COMPLETE_FAIL "Failed to mark item as complete."


/******* For memory allocation error handling *******/
#define MEM_FAIL_IN "Memory allocation failed in "
