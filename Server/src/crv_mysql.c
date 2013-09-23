/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <stdio.h>
#include <stdlib.h>

/*
 * Local STUFF
 */
#include "creuvard.h"
#include "crv_mysql.h"
#include "crv_string.h"



#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);
#define STRING_BLOCK_SIZE 1024

MYSQL * my_sql_connect(const char *server, const char *dbname, const char *user, const char *passwd)
{
	MYSQL *sql;

	if ( (sql = mysql_init(NULL)) == NULL)
	{
		fprintf(stderr, "Memory error\n");
		exit(EXIT_FAILURE);
	}
	
  mysql_options(sql, MYSQL_READ_DEFAULT_GROUP, "option");
	if (mysql_real_connect(sql, server, user, passwd, dbname, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "Connection failed on '%s'\n", server);
		fprintf(stderr, "%s", mysql_error(sql));
		free(sql);
		return NULL;
	}
	return sql;

}

void
string_my_escape (MYSQL * myh, string_t * str, const char *str2)
{
  int l;
  l = strlen (str2);
  if (str->size < (2 * l + 1))
    {
      str->size = (1 + ((2 * l + 1) / STRING_BLOCK_SIZE)) * STRING_BLOCK_SIZE;
      str->str = realloc (str->str, str->size * sizeof *str->str);
    }
  str->len = mysql_real_escape_string (myh, str->str, str2, l);
}


/* Create TABLE with hostname on BDD */
int crv_mysql_create_host_table(MYSQL *sql, const char *host)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();

	string_ajout(str, "CREATE TABLE IF NOT EXISTS `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "`(`id` int(11) NOT NULL AUTO_INCREMENT,\
  									`sha1` varchar(40) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,\
										`size` int(11) NOT NULL,\
										`name` TEXT,\
										`date_file` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
										PRIMARY KEY (`id`),\
										UNIQUE KEY `sha1` (`sha1`)\
										)");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	
	string_free(str);
	string_free(esc);
	return 0;
}

/* Create TABLE with hostname categories on BDD */
int crv_mysql_create_host_table_cat(MYSQL *sql, const char *host)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();

	string_ajout(str, "CREATE TABLE IF NOT EXISTS `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_cat`(`sha1` varchar(40) NOT NULL DEFAULT '',\
										`cat` varchar(128) NOT NULL,\
										PRIMARY KEY (`sha1`)\
										)");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	return 0;
}


/* Create TABLE with hostname groupe on BDD */
int crv_mysql_create_host_table_grp(MYSQL *sql, const char *host)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();

	string_ajout(str, "CREATE TABLE IF NOT EXISTS `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_group`(`sha1` varchar(40) NOT NULL DEFAULT '',\
										`group` varchar(128) NOT NULL,\
										PRIMARY KEY (`sha1`)\
										)");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	return 0;
}


/* Create HOST INFO on BDD */
int crv_mysql_create_host_table_info(MYSQL *sql, const char *host)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();

	/*
	 * CREATE TABLE IF NOT EXISTS `crv_127.0.0.1_info` (
	 * `host` varchar(128) NOT NULL,
	 * `bandwidth` int(10) NOT NULL,
	 * `port` int(5) NOT NULL
	 * )
	 */

	string_ajout(str, "CREATE TABLE IF NOT EXISTS `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_info`(`host` varchar(128) NOT NULL,\
										`bandwidth` int(10) NOT NULL,\
										`port` int(5) NOT NULL\
										)");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	
	string_free(str);
	string_free(esc);
	
		return 0;

}

/* Fill TABLE with hostname on BDD */
int crv_mysql_insert_host_table_file(MYSQL *sql, const char *host, const char *sha1, const char *size, const char *filename)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();
	/*
	 * INSERT INTO `crv`.`crv_127.0.0.1` (
	 * `id` ,
	 * `sha1` ,
	 * `size`
	 * )
	 * VALUES (
	 * NULL , 'fffrtgtfrdeszqazfrfffèvvvervtyurdrrfeyrd', '187625'
	 * );
	 */

	string_ajout(str, "INSERT INTO `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "` (`id`, `sha1`, `size`, `name`) VALUES ( NULL, '");
	
	string_my_escape(sql, esc, sha1);
	string_ajout(str, esc->str);
	string_ajout(str, "', '");

	string_my_escape(sql, esc, size);
	string_ajout(str, esc->str);
	string_ajout(str, "', '");
	
	string_my_escape(sql, esc, filename);
	string_ajout(str, esc->str);
	string_ajout(str, "')");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			if (mysql_errno(sql) == 1062)
			{
				return 2;
			}
			fprintf(stderr, "Echec: erreur num %d\n", mysql_errno(sql));
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	
	return 0;
}

/* Fill TABLE with hostname_cat on BDD */
int crv_mysql_insert_host_table_cat(MYSQL *sql, const char *host, const char *sha1, const char *cat)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();
	/*
	 * INSERT INTO `crv_127.0.0.1_cat` (`sha1`, `cat`) VALUES
	 * ('uygfe', '[Séries]');
	 */

	string_ajout(str, "INSERT INTO `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_cat` (`sha1`, `cat`) VALUES ('");
	
	string_my_escape(sql, esc, sha1);
	string_ajout(str, esc->str);
	string_ajout(str, "', '");

	string_my_escape(sql, esc, cat);
	string_ajout(str, esc->str);
	string_ajout(str, "')");
	
	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: erreur num %d\n", mysql_errno(sql));
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	
	return 0;
}

/* Fill TABLE with hostname_grp on BDD */
int crv_mysql_insert_host_table_grp(MYSQL *sql, const char *host, const char *sha1)
{
	string_t *str;
	string_t *esc;

	str = string_new();
	esc = string_new();
	/*
	 * INSERT INTO `crv_127.0.0.1_grp` (`sha1`, `grp`) VALUES
	 * ('uygfe', 'public');
	 */

	string_ajout(str, "INSERT INTO `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_group` (`sha1`, `group`) VALUES ('");
	
	string_my_escape(sql, esc, sha1);
	string_ajout(str, esc->str);
	string_ajout(str, "', 'public')");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: erreur num %d\n", mysql_errno(sql));
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	
	return 0;
}

/* INSERT TABLE with hostname_info on BDD */
int crv_mysql_insert_host_table_info(MYSQL *sql, const char *host, int bandwidth, int ports)
{
	string_t *str;
	string_t *esc;
	char value[10];

	str = string_new();
	esc = string_new();

	/*
	 * INSERT INTO `crv_127.0.0.1_info` (`host`, `bandwidth`, `port`) VALUES
	 * ('127.0.0.1', 850, 1664);
	 */

	string_ajout(str, "INSERT INTO `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_info` (`host`, `bandwidth`, `port`) VALUES ('");
	
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "',");

	snprintf(value, 10, "%d", bandwidth);
	string_my_escape(sql, esc, value);
	string_ajout(str, esc->str);
	string_ajout(str, ", ");

	snprintf(value, 10, "%d", ports);
	string_my_escape(sql, esc, value);
	string_ajout(str, esc->str);
	string_ajout(str, ")");
	
	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: erreur num %d\n", mysql_errno(sql));
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	
	return 0;
}

int crv_mysql_truncate_host_tables(MYSQL *sql, const char *host)
{
	string_t *str;
	string_t *esc;
	
	/*
	 * TRUNCATE `crv_127.0.0.1`;
	 */
	str = string_new();
	esc = string_new();

	string_ajout(str, "TRUNCATE  `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "`;");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);

	/*
	 * TRUNCATE `crv_127.0.0.1_cat`;
	 */
	str = string_new();
	esc = string_new();

	string_ajout(str, "TRUNCATE `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_cat`;");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);

	/*
	 * TRUNCATE `crv_127.0.0.1_info`;
	 */
	str = string_new();
	esc = string_new();

	string_ajout(str, "TRUNCATE `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_info`;");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);

	/*
	 * TRUNCATE `crv_127.0.0.1_grp`;
	 */
	
	str = string_new();
	esc = string_new();

	string_ajout(str, "TRUNCATE `crv_");
	string_my_escape(sql, esc, host);
	string_ajout(str, esc->str);
	string_ajout(str, "_group`;");

	if (0 != mysql_query(sql, (const char *)str->str))
		{
			fprintf(stderr, "Echec: '%s' %s\n", str->str, mysql_error(sql));
			mysql_close(sql);
			exit(EXIT_FAILURE);
		}
	string_free(str);
	string_free(esc);
	
	

	return 0;
}
