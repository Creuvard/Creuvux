/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef GET_H
#define GET_H
#include <sqlite3.h>

extern int Get(sqlite3 *, const char *);
extern void aff_time (int , int , int , double , double );

#endif /* GET_H */
