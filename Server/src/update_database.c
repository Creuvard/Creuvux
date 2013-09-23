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



#include <sqlite3.h>

#include "creuvard.h"
#include "update_database.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

static int check_path_cb (void *, int , char **, char **);
static int check_file_cb (void *, int , char **, char **);

/****************************************/
static int file_flag;

int check_file_cb (void *NotUsed, int argc, char **argv, char **azColName){
	//file_flag = 1;
	int i;
	struct stat buf;
	char *file;

	for(i=0; i<argc; i++){
		if (!crv_strncmp(azColName[i], "Name"))
		{
			file = crv_strdup(argv[i]);
		}
        	if (!crv_strncmp(azColName[i], "Date"))
		{
			
			if (stat(file, &buf)) {
				fprintf(stderr, "check_file_exist() failed\n");
				return (file_flag);
			}
			
			if (!crv_strncmp(argv[i], ctime(&buf.st_mtime)))
				//printf("Le fichier est le même\n");
				file_flag = 1;
		}
  	}
	return 0;
}

int check_file_exist(const char *file, sqlite3 *db)
{
	char *req = NULL;
	char *zErrMsg = NULL;
	int rc = -1;
	file_flag = 0;

	req = sqlite3_mprintf("SELECT DISTINCT Sha1,Name,Date FROM Files WHERE Name like '%q'", file);
	rc = sqlite3_exec(db, req, check_file_cb, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);
	return (file_flag);
}

/**************************************/
static char *sha1;
static int path_flag;


int check_path_cb (void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	char *path = NotUsed;
  	
	for(i=0; i<argc; i++){
        if (!crv_strncmp(azColName[i], "Sha1"))
			sha1 = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Path"))
            if (!crv_strncmp(argv[i], path))
				path_flag = 1;
  	}
	return 0;
}


/*
 * Update PATH and Category in DataBase if path as changed
 */
int check_same_path(const char *file, const char *path, sqlite3 *db)
{
	char *req = NULL;
	char *zErrMsg = NULL;
	int rc = -1;
	path_flag = 0;
	sha1 = NULL;

	req = sqlite3_mprintf("SELECT DISTINCT Sha1,Path FROM Files WHERE Name like '%q'", file);
	rc = sqlite3_exec(db, req, check_path_cb, path, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);
	
	/*
	 * If Path is different, we update Path in DB.
	 * Then we update Category in DB
	 */
	if (path_flag == 0) {
		if (strlen(path) == 0)
			req = sqlite3_mprintf("UPDATE Files SET Path='/' WHERE Sha1 like '%q'", sha1);
		else
			req = sqlite3_mprintf("UPDATE Files SET Path='%q' WHERE Sha1 like '%q'", path, sha1);
		
		rc = sqlite3_exec(db, req, NULL, 0, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		sqlite3_free(req);
		/* Update cat */
		
		char *cat = NULL; 
		cat = crv_last_dirname(path);
		if (cat == NULL)
			cat = crv_file_type (file);
		req = sqlite3_mprintf("UPDATE Categories SET Cat='%q' WHERE Sha1 like '%q'", cat, sha1);
		rc = sqlite3_exec(db, req, NULL, 0, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		sqlite3_free(req);
		crv_free(cat);
		

		crv_free(sha1);
	}

	return (path_flag);
}

/**************************************************/

int get_cat_and_change_directory(const char *file, sqlite3 *db, const char *upload_dir)
{
	fprintf(stdout, "File: '%s'\n"  , file);
	fprintf(stdout, "UP_DIR: '%s'\n", upload_dir);	
	return 0;
}
