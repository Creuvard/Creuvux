/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef CREATE_DATABASE_H
#define CREATE_DATABASE_H
#include <sqlite3.h>

extern int list_directory_and_create_db(const char *, sqlite3 *);

#endif /* CREATE_DATABASE_H */

