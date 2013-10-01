/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SQLITE3 Stuff */
#include <sqlite3.h>

#include "creuvard.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);
static char *Sha1;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
	char *a = NULL;
  for(i=0; i<argc; i++){
		if (!crv_strncmp(azColName[i], "Sha1"))
			Sha1 = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Size")){
			char *ep;
			char mo[SIZE];
			long lval;
			lval = atoi(argv[i]);
			if (lval > 1024*1024*1024)
				(void)snprintf(mo, sizeof(mo), "%ld Go", lval/(1024*1024*1024));
			else if (lval > 1024*1024)
				(void)snprintf(mo, sizeof(mo), "%ld Mo", lval/(1024*1024));
			else 
				(void)snprintf(mo, sizeof(mo), "%ld Ko", lval/(1024));
			printf("%s\t: %s\n", azColName[i], mo);	
		}
		else
			printf("%s\t: %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  return 0;
}

static int Msg_callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
	char *msg = NULL;
	char *date = NULL;
	char *pseudo = NULL;
  for(i=0; i<argc; i++){
		if (!crv_strncmp(azColName[i], "Descr"))
			msg = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Pseudo"))
			pseudo = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Date"))
			date = crv_strdup(argv[i]);
	}
	
	fprintf(stdout, "[%s] (%s):\n%s\n", pseudo, date, msg);
	
	if(msg != NULL)
		crv_free(msg);
	if(pseudo != NULL)
		crv_free(pseudo);
	if(date != NULL)
		crv_free(date);

  return 0;
}

int Info(sqlite3 *db, const char *keyword)
{
	char *zErrMsg = 0;
	int rc = -1;
	char *req = NULL;
	Sha1 = NULL;

	req = sqlite3_mprintf("SELECT DISTINCT Files.Name,Files.Sha1,Files.Size FROM Files,Categories WHERE Files.rowid='%q';", keyword);
  rc = sqlite3_exec(db, req, callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);

	req = sqlite3_mprintf("SELECT Categories.Cat FROM Categories WHERE Categories.Sha1='%q';", Sha1);
  rc = sqlite3_exec(db, req, callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);

	fprintf(stdout, "%s", "Description:\n");
	req = sqlite3_mprintf("SELECT Descr.Descr,Descr.Pseudo,Descr.Date FROM Descr WHERE Descr.Sha1='%q' ORDER BY Date ASC", Sha1);
  rc = sqlite3_exec(db, req, Msg_callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);

	return (0);
}
