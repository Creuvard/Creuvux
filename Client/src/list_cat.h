/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef LIST_CAT_H
#define LIST_CAT_H
#include <sqlite3.h>

extern char *List_cat_for_upload(sqlite3 *);
extern void List_cat(sqlite3 *);

#endif /* LIST_CAT_H */
