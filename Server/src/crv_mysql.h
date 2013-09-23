
/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef CRV_MYSQL_H
#define CRV_MYSQL_H

#include <stdio.h>
//#include <mysql.h>
#include <mysql/mysql.h>

#include "crv_mysql.h"
#include "crv_string.h"

extern MYSQL * my_sql_connect(const char *, const char *, const char *, const char *);
extern void string_my_escape (MYSQL *, string_t *, const char *);
extern int crv_mysql_insert_host_table_file(MYSQL *sql, const char *, const char *, const char *, const char *);

#endif /* CRV_MYSQL_H */
