/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

/*
 * SQLITE3 Stuff
 */

#include <sqlite3.h>

/*
 * Local STUFF
 */
#include "server_conf.h"
#include "creuvard.h"
#include "create_database.h"
#include "update_database.h"
#include "crv_mysql.h"
#include "crv_string.h"


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);



/* Server configuration options. */
ServerOptions options;

int nb_file;
int pos_file;

MYSQL *crv_mysql;

/*
 * Make action on file
 */
static void
action_file (char *root, const char *file, sqlite3 *db) 
{
	char size [12];
	struct stat buf;
	char *sha1 = NULL;
	char *cat = NULL;
	char *zErrMsg = 0;
	int rc = -1;
	char *path = NULL;
	int result = -1;
	
	if (-1 == stat(file, &buf)) 
	{
		fprintf(stderr, "%s %d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
	if (snprintf(size, 12, "%ld", (long)buf.st_size) == -1)
	{
		fprintf(stderr, "%s %d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	if (!crv_strncmp(file, "creuvux.log"))
	  return;
	else if (!strncmp(file, ".", strlen(".")))
		return;
	else if (buf.st_size > options.db_max*1024*1024)
		return;
	else if (buf.st_size < options.db_min*1024)
		return;
	else
	{	

		if (buf.st_size < 1024)
			fprintf(stdout, "%d/%d\t(%.f%%)\t--> %s (%.1f Ko)\n", pos_file, nb_file, ((float)pos_file/(float)nb_file)*100, file, (float)buf.st_size/(1024));
		else if (buf.st_size < (1024*1024*1024))
			fprintf(stdout, "%d/%d\t(%.f%%)\t--> %s (%.1f Mo)\n", pos_file, nb_file, ((float)pos_file/(float)nb_file)*100, file, (float)buf.st_size/(1024*1024));
		else if (buf.st_size < (1024*1024*1024*1024))
			fprintf(stdout, "%d/%d\t(%.f%%)\t--> %s (%.1f Go)\n", pos_file, nb_file, ((float)pos_file/(float)nb_file)*100, file, (float)buf.st_size/(1024*1024*1024*1024));
		

		sha1 = crv_sha1( file);
		if (sha1 == NULL)
			return;

		result = crv_mysql_insert_host_table_file(crv_mysql, options.host, sha1, size, file);
		if (-1 == result)
			{
				fprintf(stderr, "crv_mysql_insert_host_table_file(%s, %s, %s, %s) fail\n", options.host, file, sha1, size);
				return -1;
			}
		else if (2 == result )
			{
				return;
			}
		
		/* 
		 * INSERT file's group in BDD 
		 */
		if (-1 == crv_mysql_insert_host_table_grp(crv_mysql, options.host, sha1))
			{
				return -1;
			}

        
		/* insert into Files (Sha1,Name,Size,Path) VALUES ('<Sha1>', '<Name>', '<Size>', '<Path>'); */
		
		if (strlen(root) == 0)
			{  
		  	path = sqlite3_mprintf("INSERT INTO Files (Sha1,Name,Size,Path) VALUES ('%q', '%q', '%q', '/')", sha1, file, size);
			}
		else
			{
		  	path = sqlite3_mprintf("INSERT INTO Files (Sha1,Name,Size,Path) VALUES ('%q', '%q', '%q', '%q')", sha1, file, size, root);
			}
		
		rc = sqlite3_exec(db, path , NULL, 0, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
			
		sqlite3_free(path);
		
		
    /* Cat find in path /path-1/path-2/path-3/file = Cat path-2 */
		char **catpath;
		catpath = NULL;
		int inccat=1;
		int nb_rep = -1;
		int i = -1;
		
		if (NULL == (catpath = malloc (SIZE * sizeof *catpath)))
			return (NULL);
		catpath = crv_cut(root, "/");
		for (i = 0; catpath[i]; i++)
			{
				nb_rep++;
			}
		cat = crv_strdup(catpath[nb_rep]);
		

		/*
		 * INSERT file's cat 
		 */
		if (-1 == crv_mysql_insert_host_table_cat(crv_mysql, options.host, sha1, cat))
			{
				return -1;
			}

		if (cat != NULL)		
			crv_free(cat);
		
			
		for (i = 0; catpath[i]; i++)
			free (catpath[i]);
		
		if (sha1 != NULL)
			crv_free(sha1);
	}
	file = NULL;
	pos_file++;
}       


/*
 * List file with recursive option
 */

typedef struct slist_t
{
  char *name;
  int is_dir;
  struct slist *next;
} slist_t;


static int
recursive_dir (char *root, char *dirname, sqlite3 *db)
{
  slist_t *names = NULL;
  slist_t *sl;
  DIR * FD;
  struct dirent *f;
  int cwdlen = 32;
  char *cwd;
  char *new_root;
  
  
  cwd = crv_malloc (cwdlen * sizeof *cwd);
   
/* Concaténation new_root = "root/dirname" */
  if (root)
    {
      int rootlen = strlen (root);
      int dirnamelen = strlen (dirname);
	  new_root = crv_malloc ((rootlen + dirnamelen + 2) * sizeof *new_root);
      memcpy (new_root, root, (size_t)rootlen);
      new_root[rootlen] = '/';
      memcpy (new_root + rootlen + 1, dirname, (size_t)dirnamelen);
      new_root[rootlen + dirnamelen + 1] = '\0';
    }
  else
    new_root = crv_strdup (dirname);
   
/* Obtention du répertoire courant */
  while (NULL == (cwd = getcwd (cwd, (size_t)cwdlen)))
    {
      if ( ERANGE != errno )
        {
          fprintf (stderr, "Problème avec getcwd (errno = '%s')\n",
                   strerror (errno));
          exit (EXIT_FAILURE);
        }
      cwdlen += 32;
      cwd = realloc (cwd, cwdlen * sizeof *cwd);
    }
	crv_chdir (dirname);
/* Remplissage de la liste chaînée avec les noms des fichiers* du répertoire courant */
  if (NULL == (FD = opendir (".")))
    {
      fprintf (stderr, "opendir() impossible\n");
      return (-1);
    }
  sl = names;
  while ((f = readdir (FD)))
    {
      struct stat st;
      slist_t *n;
      if (!strcmp (f->d_name, "."))
        continue;
      if (!strcmp (f->d_name, ".."))
        continue;
			if (!strncmp (f->d_name, ".", strlen(".")))
				continue;
      if (stat (f->d_name, &st))
        continue;
	  n = crv_malloc (sizeof *n);
      
	  n->name = strdup (f->d_name);
      if (S_ISDIR (st.st_mode))
        n->is_dir = 1;
      else
        n->is_dir = 0;
      n->next = NULL;
      if (sl)
        {
          sl->next = n;
          sl = n;
        }
      else
        {
          names = n;
          sl = n;
        }
    }
  closedir (FD);
   
/* Parcourt les fichiers et répertoires pour action */
  for (sl = names; sl; sl = (slist_t *)sl->next)
    {
      if (!sl->is_dir) 
		action_file (new_root, sl->name, db);
    }
   
/* Parcourt les fichiers et répertoires pour action avant traitement récursif,
 * traitement récursif, et action après traitement récursif.
 */
  for (sl = names; sl; sl = (slist_t *)sl->next)
  {
		if (sl->is_dir) {
			if ( strcmp( sl->name, "UPLOAD") )
				recursive_dir (new_root, sl->name, db);
		}
	}
   
/* Nettoyage */
  free (new_root);
  while (names)
    {
      slist_t *prev;
      free (names->name);
      prev = names;
      names = (slist_t *)names->next;
      free (prev);
    }
  chdir (cwd);
  free (cwd);
  return (0);
}

static int
recursive_compt_dir (char *root, char *dirname)
{
  slist_t *names = NULL;
  slist_t *sl;
  DIR * FD;
  struct dirent *f;
  int cwdlen = 32;
  char *cwd;
  char *new_root;
  
  cwd = crv_malloc (cwdlen * sizeof *cwd);
   
/* Concaténation new_root = "root/dirname" */
  if (root)
    {
      int rootlen = strlen (root);
      int dirnamelen = strlen (dirname);
	  new_root = crv_malloc ((rootlen + dirnamelen + 2) * sizeof *new_root);
      memcpy (new_root, root, (size_t)rootlen);
      new_root[rootlen] = '/';
      memcpy (new_root + rootlen + 1, dirname, (size_t)dirnamelen);
      new_root[rootlen + dirnamelen + 1] = '\0';
    }
  else
    new_root = crv_strdup (dirname);
   
/* Obtention du répertoire courant */
  while (NULL == (cwd = getcwd (cwd, (size_t)cwdlen)))
    {
      if ( ERANGE != errno )
        {
          fprintf (stderr, "Problème avec getcwd (errno = '%s')\n",
                   strerror (errno));
          exit (EXIT_FAILURE);
        }
      cwdlen += 32;
      cwd = realloc (cwd, cwdlen * sizeof *cwd);
    }
	crv_chdir (dirname);
   
/* Remplissage de la liste chaînée avec les noms des fichiers* du répertoire courant */
  if (NULL == (FD = opendir (".")))
    {
      fprintf (stderr, "opendir() impossible\n");
      return (-1);
    }
  sl = names;
  while ((f = readdir (FD)))
    {
      struct stat st;
      slist_t *n;
      if (!strcmp (f->d_name, "."))
        continue;
      if (!strcmp (f->d_name, ".."))
        continue;
      if (!strncmp (f->d_name, ".", strlen(".")))
				continue;
			if (stat (f->d_name, &st))
        continue;
	  n = crv_malloc (sizeof *n);
      
	  n->name = strdup (f->d_name);
      if (S_ISDIR (st.st_mode))
        n->is_dir = 1;
      else
        n->is_dir = 0;
      n->next = NULL;
      if (sl)
        {
          sl->next = n;
          sl = n;
        }
      else
        {
          names = n;
          sl = n;
        }
    }
  closedir (FD);
   
/* Parcourt les fichiers et répertoires pour action */
  for (sl = names; sl; sl = (slist_t *)sl->next)
    {
      if (!sl->is_dir)
	  {
		struct stat buf;
		if (-1 == stat(sl->name, &buf)) 
		{
			fprintf(stderr, "%s %d\n", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (strncmp( sl->name, ".", strlen(".")) && crv_strncmp( sl->name, "creuvux.log") && (buf.st_size < options.db_max*1024*1024) && (buf.st_size > options.db_min*1024))
			nb_file++;
	  }
    }
   
/* Parcourt les fichiers et répertoires pour action avant 
 * traitement récursif, et action après traitement récursif.
 */
  for (sl = names; sl; sl = (slist_t *)sl->next)
  {
		if (sl->is_dir)
			recursive_compt_dir (new_root, sl->name);
	}
   
/* Nettoyage */
  free (new_root);
  while (names)
    {
      slist_t *prev;
      free (names->name);
      prev = names;
      names = (slist_t *)names->next;
      free (prev);
    }
  chdir (cwd);
  free (cwd);
  return (nb_file);
}



/* 
 * Lis and update SQLITE3 database
 */
int list_directory_and_create_db(const char *dir, sqlite3 *db)
{
	int result = -1;
	pos_file = 1;
	nb_file = 0;
	
	debug();
	/* Connect to MySQL DataBase*/
	crv_mysql = my_sql_connect(options.ipdb, options.dbname, options.dbuser, options.dbpassw);
	debug();


	/* Create on MySQL crv_'hostname' */
	if (-1 == crv_mysql_create_host_table(crv_mysql, options.host))
		return -1;
	
	/* Create on MySQL crv_'hostname'_cat */
	if (-1 == crv_mysql_create_host_table_cat(crv_mysql, options.host))
		return -1;

	/* Create on MySQL crv_'hostname'_grp */
	if (-1 == crv_mysql_create_host_table_grp(crv_mysql, options.host))
		return -1;

	/* Create on MySQL crv_'hostname'_info */
	if (-1 == crv_mysql_create_host_table_info(crv_mysql, options.host))
		return -1;
	
	/* TRUNCATE on MySQL all table crv_'host' crv_'host'_cat and crv_'host'_info if they are not empty */
	if (-1 == crv_mysql_truncate_host_tables(crv_mysql, options.host))
		return -1;

	/* INSERT info about hostname */
	if (-1 == crv_mysql_insert_host_table_info(crv_mysql, options.host, options.bandwidth, options.num_ports))
		return -1;
	
	//result = recursive_compt_dir ( NULL, (char *)"");
	result = recursive_compt_dir ( NULL, (char *)dir);
	if ((result == -1)) {
	  fprintf(stderr, "%s\n", "list_directory_and_create_db(): Err[001] recursive_compt_dir() failed ");
	  return (-1);
	}

	result = recursive_dir ( NULL, (char *)dir, db);
	//result = recursive_dir ( NULL, (char *)"", db);
	if ((result == -1)) {
	  fprintf(stderr, "%s\n", "list_directory_and_create_db(): Err[002] recursive_dir() failed ");
	  return (-1);
	}
	
	mysql_close(crv_mysql);
	return 0;
}

