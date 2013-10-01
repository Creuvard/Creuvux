/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef LIST_GRP_H
#define LIST_GRP_H
#include <sqlite3.h>

extern char *List_group_for_upload(sqlite3 *);
extern void List_group(sqlite3 *);

#endif /* LIST_GRP_H */
