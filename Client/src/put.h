/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#ifndef PUT_H
#define PUT_H
#include <sqlite3.h>

extern void pre_Put(sqlite3 *, char **);
extern void upload(char *);
//extern int  Put(const char *, const char *, const char *, const char *);

//extern char *give_server(void);

#endif /* PUT_H */
