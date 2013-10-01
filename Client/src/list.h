/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef LIST_H
#define LIST_H
#include <sqlite3.h>

extern int List_callback(void *, int , char **, char **);
extern void print_headers_table(void);
extern void print_end_table(void);
extern int init_table (void *, int , char **, char **);
extern int List(sqlite3 *);

#endif /* LIST_H */
