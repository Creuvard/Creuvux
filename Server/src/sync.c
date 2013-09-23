/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>


/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

/*
 * SQLite3 Stuff
 */
#include <sqlite3.h>


#include "creuvard.h"
#include "network.h"
#include "log.h"
#include "server_conf.h"
//#include "xml_data.h"
#include "SSL_sendfile.h"
//#include "creuvux_stat.h"
//#include "msg.h"
//#include "new_file.h"
#include "sync.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Server configuration options. */
extern ServerOptions options;



/* Command options. */
Command_options command;

void pre_Sync(void)
{
	char lst_file[SIZE];

	command.name = crv_strdup("SYNC");
 
	(void)crv_strncpy(lst_file, "/.", sizeof(lst_file));
  (void)crv_strncat(lst_file, command.user, sizeof(lst_file));
  (void)crv_strncat(lst_file, ".db", sizeof(lst_file));

  command.lst_file = crv_strdup( lst_file);
  memset (lst_file, 0, sizeof(lst_file));
}

static int Sync_cb_Files(void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
  char *Name = NULL;
	char *Size = NULL;
	char *Date = NULL;
  char *req = NULL;
  sqlite3 *db_result = NULL;
  db_result = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "Name"))
      Name = strdup(argv[i]);

		if (!strcmp((const char *)azColName[i], "Size"))
      Size = strdup(argv[i]);

		if (!strcmp((const char *)azColName[i], "Date"))
      Date = strdup(argv[i]);

  }

	req = sqlite3_mprintf("INSERT into Files  (Sha1,Name,Size,Date) values ('%q', '%q', '%q', '%q');", Sha1, Name, Size, Date);

  rc = sqlite3_exec(db_result, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Name != NULL)
		crv_free(Name);
	if ( Size != NULL)
		crv_free(Size);
	if (Date != NULL)
		crv_free(Date);

  return 0;
}


static int Sync_cb_Cat(void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
	char *Cat = NULL;
  char req[1024];
  sqlite3 *db_result = NULL;
  db_result = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "Cat"))
      Cat = strdup(argv[i]);
  }

  snprintf(req, sizeof(req), "INSERT into Categories (Sha1,Cat) values ('%s', \"%s\");" , Sha1, Cat);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Cat != NULL)
		crv_free(Cat);

  rc = sqlite3_exec(db_result, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}


static int Sync_cb_Grp_Sha1 (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
	char *Grp = NULL;
  char req[1024];
  sqlite3 *db_result = NULL;
  db_result = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "groupname"))
      Grp = strdup(argv[i]);
  }

  snprintf(req, sizeof(req), "INSERT into Grp_Sha1 (Sha1,Groupname) values ('%s', '%s');" , Sha1, Grp);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Grp != NULL)
		crv_free(Grp);

  rc = sqlite3_exec(db_result, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}

static int Sync_cb_Descr (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
	char *Descr = NULL;
	char *Pseudo = NULL;
	char *Date = NULL;
  char *req = NULL;
  sqlite3 *db_result = NULL;
  db_result = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);

    if (!strcmp((const char *)azColName[i], "Descr"))
      Descr = strdup(argv[i]);
		if (!strcmp((const char *)azColName[i], "Pseudo"))
      Pseudo = strdup(argv[i]);
		if (!strcmp((const char *)azColName[i], "Date"))
      Date = strdup(argv[i]);
  }

  req = sqlite3_mprintf("INSERT into Descr (Sha1,Descr,Pseudo,Date) values ('%q', '%q', '%q', '%q');", Sha1, Descr, Pseudo, Date);
	if ( Sha1 != NULL)
		crv_free(Sha1);
	if ( Descr != NULL)
		crv_free(Descr);
	if ( Pseudo != NULL)
		crv_free(Pseudo);
	if ( Date != NULL)
		crv_free(Date);

  rc = sqlite3_exec(db_result, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}

static int Sync_cb_Server (void *db, int argc, char **argv, char **azColName){
  int i;
  char *zErrMsg = 0;
  int rc = -1;
  char *Sha1 = NULL;
  char req[4096];
  sqlite3 *db_result = NULL;
  db_result = db;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    //printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
    if (!strcmp((const char *)azColName[i], "Sha1"))
      Sha1 = strdup(argv[i]);
  }

	snprintf(req, sizeof(req), "INSERT into Server (Sha1 , Servername, Port, bandwidth) values ('%s', '%s', '%d', '%d');" , Sha1, options.host, options.num_ports, options.bandwidth);
	if ( Sha1 != NULL)
		crv_free(Sha1);

  rc = sqlite3_exec(db_result, req , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return 0;
}


int Sync(SSL *ssl)
{
	int nb_write = -1;
	int result = -1;
	off_t size = 0;
	sqlite3 *db = NULL;
	sqlite3 *db_result = NULL;
	char *zErrMsg = 0;
	int rc = -1;
	char *req = NULL;

	/* Print Log */
	Log ("OK", 1, "REQ_LIST");
	
	/* Send SYNC_ACK flag */
	nb_write = SSL_write (ssl, "SYNC_ACK", strlen("SYNC_ACK"));
	if (nb_write <= 0) {
		Log ("WARNING", 1, "SYNC_ACK -> PB");
		return (-1);
	}

	/* Open USER.db */
	rc = sqlite3_open( command.lst_file, &db_result);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db_result));
		sqlite3_close(db_result);
		return (-1);
	}

	/* Open main database */
	rc = sqlite3_open( "/.creuvux.db", &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return (-1);
	}
	
	/* Create table Files for USER.db database */
	rc = sqlite3_exec(db_result, "create table Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL, Date DATETIME DEFAULT '0000-00-00 00:00:00' NOT NULL);" , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
  }

	/*
	 * Table: Grp_Sha1
	 * Creation de la TABLE dans laquelle sera contenue la liste des groupes/sha1 pour que le client puisse l'afficher
	 */
	rc = sqlite3_exec(db_result, "CREATE table Grp_Sha1 (Sha1 TEXT NOT NULL, groupname TEXT NOT NULL);" , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
  }


	/*
	 * Table: Categories
	 * Creation de la TABLE dans laquelle sera contenue la liste des categories/sha1
	 */
	rc = sqlite3_exec(db_result, "CREATE table Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL);" , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
  }

	/*
	 * Table: Server
	 * Info about server's files (Sha1, servername, port, bandwidth)
	 * 
	 */
	rc = sqlite3_exec(db_result, "CREATE table Server (Sha1 TEXT NOT NULL, Servername TEXT NOT NULL, Port NUMERIC NOT NULL, bandwidth NUMERIC NOT NULL);" , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
  }

	/*
	 * Table: Descr
	 */
	rc = sqlite3_exec(db_result, "CREATE table Descr (Sha1 TEXT NOT NULL, Descr TEXT NOT NULL, Pseudo TEXT NOT NULL, Date DATETIME DEFAULT '0000-00-00 00:00:00' NOT NULL);" , NULL, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
  }

	/* Fill Files table */
	/*
	 * SELECT Name,Sha1,Size from Files WHERE Sha1 IN (SELECT DISTINCT Sha1 FROM Grp_Sha1 WHERE groupname IN (SELECT DISTINCT groupname FROM Grp_User WHERE username='creuvard'));
	 *
	 */
	req = sqlite3_mprintf("SELECT DISTINCT Name,Sha1,Size,Date from Files WHERE Sha1 IN (SELECT DISTINCT Sha1 FROM Grp_Sha1 WHERE groupname IN (SELECT DISTINCT groupname FROM Grp_User WHERE username='%q'));", command.user);	
	rc = sqlite3_exec(db, req , Sync_cb_Files, (void *)db_result, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);


	/* Fill Categories table */
  req = sqlite3_mprintf("SELECT DISTINCT Sha1,Cat from Categories WHERE Sha1 IN (SELECT DISTINCT Sha1 FROM Grp_Sha1 WHERE groupname IN (SELECT DISTINCT groupname FROM Grp_User WHERE username='%q'));", command.user);	
	rc = sqlite3_exec(db, req , Sync_cb_Cat, (void *)db_result, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);



	/* Fill Grp_Sha1 table */
  req = sqlite3_mprintf("SELECT DISTINCT Sha1,groupname from Grp_Sha1 WHERE Sha1 IN (SELECT DISTINCT Sha1 FROM Grp_Sha1 WHERE groupname IN (SELECT DISTINCT groupname FROM Grp_User WHERE username='%q'));", command.user);
	rc = sqlite3_exec(db, req , Sync_cb_Grp_Sha1, (void *)db_result, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	/* Fill Server table */
  req = sqlite3_mprintf("SELECT DISTINCT Sha1 from Files WHERE Sha1 IN (SELECT DISTINCT Sha1 FROM Grp_Sha1 WHERE groupname IN (SELECT DISTINCT groupname FROM Grp_User WHERE username='%q'));", command.user);
	rc = sqlite3_exec(db, req , Sync_cb_Server, (void *)db_result, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	/* Fill Descr table */
  req = sqlite3_mprintf("SELECT DISTINCT Sha1,Descr,Pseudo,Date from Descr WHERE Sha1 IN (SELECT DISTINCT Sha1 FROM Grp_Sha1 WHERE groupname IN (SELECT DISTINCT groupname FROM Grp_User WHERE username='%q'));", command.user);
	rc = sqlite3_exec(db, req , Sync_cb_Descr, (void *)db_result, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	sqlite3_close(db);
	sqlite3_close(db_result);

	/***************************/
	/* Send database to client */
	/***************************/

	/* Get size of file */
	size = crv_du (command.lst_file);
	/* Send USER.db to client */
	result = SSL_sendfile( ssl, command.lst_file , (off_t)0, size);
	if (result == -1 ) {
		Log ("WARNING", 1, "SSL_sendfile(): Send 'username.lst' failed");
		return (-1);
	}
	else 
		Log ("OK", 1, "REQ_LIST -> OK");
	
	/* Send SYNC_END flag */
	nb_write = SSL_write (ssl, "SYNC_END\0", strlen("SYNC_END\0"));
	if (nb_write <= 0) {
		Log ("WARNING", 1, "SYNC_END -> PB");
		return (-1);
	}
	
	/* Erase USER.bd */
	result = unlink(command.lst_file);
	if (result == -1 ) {
		Log ("WARNING", 1, "traitement_conec(): unlink() failed with error ->", strerror(errno));
		return (-1);
	}

	return (0);
}
