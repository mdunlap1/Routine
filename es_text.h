/*******************************************************************************
 * es_text.h
 * Spanish language header file.
 *
 ******************************************************************************/
/* Use this file to store all the text that gets displayed so that we can 
 * change the language that the program uses */

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
#define DATE_FRMT_STR "%d-%d-%d"
#define DMY
#define DATE_STR_LIMIT 11
#define DATE_FRMT_EXPLAIN "mes/día/año"

/******* for language specific input limits *******/
// NOTE: these are not currently being used 
#define DESC_LIMIT 101
#define FREQ_TYPE_LIMIT 7
#define CATEGORY_LIMIT 21

/******* For init.c *******/
#define FATAL_DB_ERROR_NO_ACCESS "Error fatal: no puede acceder a la base de datos"

/******* For main_view.c *******/

#define TODAY_VIEW_HEADING "Cosas para hoy"

#define DESCRIPTION "Descripción"

#define DUE "Fecha de vencimiento"

#define COMPLETE_PUSH_BACK  "Completado el/\nCambiar fecha de vencimiento"

#define CATEGORY "Categoría"

#define NO_ITEMS_CURRENTLY_DUE "No hay tareas actualmente vencidos."

#define ADD "agregar"

#define EDIT_SEARCH "editar / buscar"

#define LOOK_AHEAD_OR_BACK "adelante / atrás"

#define SNOOZE "cambiar fecha"

#define MARK_COMPLETED "completar"

/******* For add_view.c *******/

// Uses DESCRIPTION

#define DUE_ON "Incialmente vence"

#define FREQUENCY "Frecuencia"

#define BACK "atrás"

#define SUBMIT "enviar"

// Uses CATEGORY

#define DAYS "días" 

#define MONTHS "meses"

#define YEARS "años"

#define NO_REPEAT "no repetir"

#define TRACK_HISTORY "¿Añadir historial?"

#define YES "sí"

#define NO "no"

#define DESCRIPTION_COLLISION "ERROR: La descripción coincide con una que ya está en la base de datos."

#define CANNOT_HAVE_APOSTROPHES "ERROR: Las descripciones y categorías no pueden tener apóstrofes"

#define INVALID_DATE "ERROR: El formato de fecha no es válido.\nDebe tener un formato así: "


/******* For look_select_view.c *******/

#define VIEW "ver"

#define FROM "Desde"

#define TO "a"

#define INCLUSIVE "incluído"

#define LOOK "Mirar"

#define AHEAD "adelante"

#define WEEKS "semanas"

#define DAY "día"

#define WEEK "semana"

#define MONTH "mes"

// Uses BACK
// Uses MONTHS
// Uses INVALID_DATE

#define START_BEFORE_END "La fecha de inicio debe ser anterior o igual a la fecha de finalización"

/******* For use in ahead_back_view.c *******/
#define MAIN "principal"
#define NO_HIST_IN_RANGE "No hay elementos completados en el rango seleccionado"
#define REMOVE "eliminar"
#define CHANGE_DATE "cambiar la fecha"
#define COMPLETED_ON "Completado el"
#define CHANGE_TO "Cambiar la fecha de finalización a"
#define NO_ITEMS_IN_RANGE "No hay elementos para mostrar en el rango seleccionado"
#define ITEMS_DUE_IN_RANGE "Tareas con vencimiento en el rango seleccionado"
#define ITEMS_COMPLETED_IN_RANGE "Tareas completados en el rango seleccionado"

/******* For use in edit_select_view.c *******/
#define SELECT "escoger"
#define REPEAT_EVERY "Se repite cada"
// Uses TRACK_HISTORY

/******* For use in selected_view.c *******/
#define UPDATE_COMPLETED "Actualización completada"
#define UPDATE_ATTRIBUTES "Actualizar atributos"
// Uses DUE_ON
#define NOT_DUE "no hay fecha de vencimiento"
#define CHANGE_DUE_DATE "cambiar la fecha de vencimiento"
#define EVERY "Cada"
#define CHANGE_FREQ "cambiar la frecuencia"
#define CHANGE_CATEGORY "cambiar la categoría"
#define REMOVE_FROM_UPCOMING "eliminar la fecha de vencimiento"
#define REMOVE_AND_DELETE_HIST "Eliminar tarea y eliminar historial"
#define PURGE_WARN "Estás seguro/segura que quieres elimiar todo la información de la tarea?"
#define PERMANENTLY_REMOVE "Eliminar permanentemente la tarea y el historial"
#define PURGE_ITEM "purgar tarea"

/******* For error handling and success *******/
#define FATAL_ERROR "Error fatal encontrado."
#define COMPLETION_ERROR "Error: No se puede cargar el modelo de finilazacíon."
#define SUCCESS "¡Éxito!"
#define PURGE_SUCCESS "Tarea purgado correctamente de la base de datos."
#define DATABASE_ERROR "Error de la base de datos!\n"
#define DATABASE_ERROR_FATAL "Error fatal de la base de datos encontrado.\n"
#define DATABASE_HIST_FAIL "Error al actualizar el estado de la tarea."
#define DATABASE_UPDATE_DUE_DATE_FAIL "Error al actualizar la fecha de vencimiento de la tarea."
#define DATABASE_FAILED_TO_GET_TRACKING "Error al obtener el seguimiento de la tarea.\n"
#define DATABASE_FAILED_TO_GET_LAST_COMPLETED "Error: No se pudo obtener la fecha de la última finalizacíon.\n"
#define DATABASE_SNOOZE_FAIL "Error: No se pudo cambiar la fecha de vincimiento."
#define DATABASE_ADD_FAIL "Error: No se pudo agregar tarea nueva."
#define DATABASE_ADD_UPCOMING_FAIL "Error: No se pudo agregar tarea a los próximos." 
#define DATABASE_CANNOT_LOAD_ATTRIBUTES_FATAL "FATAL Error: No se pudo obtener la informacíon de la tarea.\n"
#define DATABASE_FAILED_TO_CHANGE_CATEGORY "Error: No se pudo cambiar la categoría del elemento.\n Por favor, intente otra vez antes de continuar\npor la razón que la base de datos puede contener un error."
#define DATABASE_FAIL_TO_CHANGE_DUE_DATE "Error: No se pudo cambiar la fecha de finalización.\n"
#define DATABASE_FREQ_FAIL "Error: No se pudo cambiar la frecuencia."
#define DATABASE_EDIT_HIST_FAIL "Error: No se pudo cambiar la fecha de terminación." 
#define DATABASE_FAIL_REMOVE_HIST "Error: No se pudo remover la fecha de terminación."
#define DATABASE_REMOVE_UPCOMING_FAIL "Error: No se pudo remover elemento del listo de próximos."
#define DATABASE_PURGE_ITEM_FAIL "ERROR: Al intentar eliminar el elemento ocurrió un error.\nLa base de datos puede está seriamente dañada.\nPor favor solucionar antes de continuar."
#define DATABASE_ATTRIBUTES_MODEL_FAIL "Fatal Error: No se pudieron obtener los datos de elemento"
#define DATABASE_MARK_COMPLETE_FAIL "Error al marcar el elemento íntegro."


/******* For memory allocation error handling *******/
#define MEM_FAIL_IN "Error en la asignación de memoria en "
