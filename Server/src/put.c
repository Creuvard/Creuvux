/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


/*
 * SQLite3 Stuff
 */
#include <sqlite3.h>


#include "creuvard.h"
#include "network.h"
#include "log.h"
#include "server_conf.h"
#include "put.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Server configuration options. */
extern ServerOptions options;

static char *Title;
static char *Old_Sha1;
static char *pseudo_client;

static int Put_cb_Files(void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
  char *Name = NULL;
	char *Size = NULL;
  char *req = NULL;
  sqlite3 *db_dst = NULL;
  db_dst = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1")) {
      Sha1 = strdup(argv[i]);
			Old_Sha1 = strdup(argv[i]);
		}

    if (!strcmp((const char *)azColName[i], "Name")) {
      Name = strdup(argv[i]);
			Title = strdup(argv[i]);
		}

		if (!strcmp((const char *)azColName[i], "Size"))
      Size = strdup(argv[i]);

  }

	req = sqlite3_mprintf("INSERT into Files  (Sha1,Name,Size,Path,Date) values ('%q', '%q', '%q', '%q', datetime('now'));", Sha1, Name, Size, options.upload_directory);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Name != NULL)
		crv_free(Name);
	if ( Size != NULL)
		crv_free(Size);


  rc = sqlite3_exec(db_dst, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}


static int Put_cb_Cat(void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
	char *Cat = NULL;
  char *req = NULL;
  sqlite3 *db_dst = NULL;
  db_dst = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "Cat"))
      Cat = strdup(argv[i]);
  }

	req = sqlite3_mprintf("INSERT into Categories (Sha1,Cat) values ('%q', '%q');", Sha1, Cat);

	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Cat != NULL)
		crv_free(Cat);

  rc = sqlite3_exec(db_dst, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}


static int Put_cb_Grp_Sha1 (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
	char *Grp = NULL;
  char *req = NULL;
  sqlite3 *db_dst = NULL;
  db_dst = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "groupname"))
      Grp = strdup(argv[i]);
  }

	req = sqlite3_mprintf("INSERT into Grp_Sha1 (groupname, sha1) values ('%q', '%q');", Grp, Sha1);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Grp != NULL)
		crv_free(Grp);

  rc = sqlite3_exec(db_dst, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}

static int Put_cb_Grp_User (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *User = NULL;
	char *Grp = NULL;
  char *req = NULL;
  sqlite3 *db_dst = NULL;
  db_dst = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "username"))
      User = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "groupname"))
      Grp = strdup(argv[i]);
  }
	
	req = sqlite3_mprintf("INSERT into Grp_User (username,groupname) values ('%q', '%q');", User, Grp);
	if ( User != NULL)
		crv_free(User);
	if ( Grp != NULL)
		crv_free(Grp);

  rc = sqlite3_exec(db_dst, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}

static int Put_cb_Blob_Data (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
  char *blob = NULL;
  char *req = NULL;
  sqlite3 *db_dst = NULL;
  db_dst = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "Descr"))
      blob = strdup(argv[i]);
  }

  req = sqlite3_mprintf("INSERT into Blob_Data (Sha1,Blob_Data) values ('%q', '%q');", Sha1, blob);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( blob != NULL)
		crv_free(blob);

  rc = sqlite3_exec(db_dst, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}

static int Put_cb_Descr (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
	char *Descr = NULL;
  char *req = NULL;
  sqlite3 *db_dst = NULL;
  db_dst = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "Descr"))
      Descr = strdup(argv[i]);
  }

  req = sqlite3_mprintf("INSERT into Descr (Sha1,Descr,Pseudo,Date) values ('%q', '%q', '%q', datetime('now'));", Sha1, Descr, pseudo_client);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Descr != NULL)
		crv_free(Descr);

  rc = sqlite3_exec(db_dst, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}


int Add_Comment(const char *filename, const char *username)
{
	char *zErrMsg = 0;
	int rc = -1;
	sqlite3 *db_src = NULL;
	sqlite3 *db_dst = NULL;
	pseudo_client = crv_strdup(username);

	Title = NULL;
	Old_Sha1 = NULL;

	/* Open sql_file just uploaded */
	rc = sqlite3_open( filename, &db_src);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db_src));
		sqlite3_close(db_src);
		return (-1);
	}

	/* Open main database */
	rc = sqlite3_open( "/.creuvux.db", &db_dst);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db_dst));
		sqlite3_close(db_dst);
		return (-1);
	}

	/*******************************************************************************************/

	/* Fill Files table */
	rc = sqlite3_exec(db_src, "SELECT * FROM Files;" , Put_cb_Files, (void *)db_dst, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* Fill Categories table */
	rc = sqlite3_exec(db_src, "SELECT * FROM Categories;" , Put_cb_Cat, (void *)db_dst, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}


	/* Fill Grp_Sha1 table */
	rc = sqlite3_exec(db_src, "SELECT * FROM Grp_Sha1;" , Put_cb_Grp_Sha1, (void *)db_dst, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* Fill Grp_User table */
	rc = sqlite3_exec(db_src, "SELECT * FROM Grp_User" , Put_cb_Grp_User, (void *)db_dst, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* Fill Descr table */
	rc = sqlite3_exec(db_src, "SELECT * FROM Descr" , Put_cb_Descr, (void *)db_dst, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* Fill Blob_Data table */
	/*
	rc = sqlite3_exec(db_src, "SELECT * FROM Blob_Data" , Put_cb_Blob_Data, (void *)db_dst, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	*/
	/*******************************************************************************************/


	sqlite3_close(db_src);
	sqlite3_close(db_dst);
	
	if(pseudo_client != NULL)
		crv_free(pseudo_client);

	rc = rename( Old_Sha1, Title);
  if (rc == -1) {
    Log ("WARNING", 1, "COMMENT -> Can't rename( listing_tmp, up_file) ", strerror(errno));
    return (-1);
  }

  /*
	rc = unlink((const char *)filename);
  if (rc == -1)
		fprintf(stderr, "%s\n", "Msg_Reply(): Err[001] unlink() failed");
	*/
	/* Chdir ian chroot directory */
	rc = crv_chdir("/");
	if (rc == -1) {
		Log ("WARNING", 1, "traitement_conec(): crv_chdir(upload_directory) failed");
		return (-1);
	}
	
	return (0);
}
