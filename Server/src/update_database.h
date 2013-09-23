/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef UPDATE_DATABASE_H
#define UPDATE_DATABASE_H
#include <sqlite3.h>

extern int check_file_exist(const char *, sqlite3 *);
extern int check_same_path(const char *, const char* , sqlite3*);
extern int get_cat_and_change_directory(const char *, sqlite3 *, const char *);

#endif /* UPDATE_DATABASE_H */

