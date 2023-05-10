SQL = -lsqlite3
objects = helpers main_view dates init sql_db add look_select ahead_back edit_select selected
GTK = `pkg-config --cflags --libs gtk+-3.0`
LANGUAGES = text_en.h es_text.h

routine : $(objects) 
	gcc $(SQL) $(GTK) $(objects) -o routine 

init : init.c setup.h sql_db.h 
	gcc $(SQL) $(GTK) -c -o init init.c 

main_view : main_view.c dates.h helpers.h main_enum.h setup.h sql_db.h 
	gcc $(SQL) $(GTK) -c -o main_view main_view.c  

add : add_view.c dates.h helpers.h setup.h sql_db.h 
	gcc $(SQL) $(GTK) -c -o add add_view.c 

look_select : look_select_view.c dates.h helpers.h setup.h sql_db.h
	gcc $(GTK) $(SQL) -c -o look_select look_select_view.c

ahead_back : ahead_back_view.c dates.h helpers.h main_enum.h setup.h sql_db.h 
	gcc $(GTK) $(SQL) -c -o ahead_back ahead_back_view.c

edit_select : edit_select_view.c setup.h sql_db.h
	gcc $(GTK) $(SQL) -c -o edit_select edit_select_view.c

selected : selected_view.c dates.h helpers.h main_enum.h setup.h sql_db.h
	gcc $(GTK) -c -o selected selected_view.c


helpers : helpers.c main_enum.h setup.h
	gcc $(GTK) -c -o helpers helpers.c

dates : dates.c dates.h setup.h
	gcc $(GTK) -c -o dates dates.c

sql_db : sql_db.c dates.h helpers.h main_enum.h setup.h sql_db.h
	gcc $(SQL) $(GTK) -c -o sql_db sql_db.c 

setup.h : $(LANGUAGES) 
	echo "setup.h has had a language file modified"
	touch setup.h

create : create_db.c
	gcc -lsqlite3 -o create create_db.c

clean :
	rm -f $(objects) routine create
