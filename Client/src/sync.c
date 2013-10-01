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

#include <sys/types.h>
#include <sys/stat.h>

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
 * SQLITE3 Stuff
 */

#include <sqlite3.h>

#include <pthread.h>

#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

static void 
receive_sync_file (void *arg)
{
  int sd;
  int code = 0;
  int port;
  int i;
  long lval;
  char command[SIZE];
  char *sync_arg = arg;
  char buf[4096];
  char *ep = NULL;
  char host_port[SIZE];
  char **c = NULL;
  FILE *fd = NULL;
  SSL_CTX* ctx;
  SSL*     ssl;


  c = crv_cut (sync_arg, ":");
  lval = strtol(c[1], &ep, 10);
  if (c[0][0] == '\0' || *ep != '\0') {
    fprintf(stderr, "%s\n", "Sync(): Err[002] Check port on serveur_liste");
    return ;
  }
  if (lval > 65535 || lval < 0) {
    fprintf(stderr, "%s\n", "Sync(): Err[003] Check port on serveur_liste");
    return ;
  }
  port = lval;

  crv_strncpy (host_port, "./listing/", sizeof(host_port));
  crv_strncat (host_port, c[0], sizeof(host_port));	/* put server name in host_port */
  crv_strncat (host_port, ".db", sizeof(host_port));	/* add : behind server name */
  fprintf (stdout, "%s", "Listing reception on ");
  fflush(stdout);

#ifndef WIN32
      couleur ("33;31");
#else
      couleur (12, 0);
#endif
      fprintf (stdout, "'%s'", c[0]);
	  fflush(stdout);
#ifndef WIN32
      couleur ("0");
#else
      couleur (15, 0);
#endif
      fprintf (stdout, "%s\n", " is running ...");

  /* SSL preliminaries */
  init_OpenSSL ();			  
  seed_prng ();
  
  sd = crv_client_create(port, c[0], options.address_family);
	if (sd == -1) {
		close(sd);
		pthread_exit(NULL);
	}

  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_sync_file() Err[001] Create new ssl failed");
	pthread_exit(NULL);
  }
		
  /* connect the SSL object with a file descriptor */
  code = SSL_set_fd (ssl, sd);
  if ( code == 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_sync_file() Err[002] Put SSL on socket failed \n");
	pthread_exit(NULL);
  }

  code = SSL_connect (ssl);
  if (code == 0)	{
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "receive_sync_file(): Err[003] SSL_connect() failed");
	pthread_exit (NULL);
  } else
  if (code == -1) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "receive_sync_file(): Err[004] SSL_connect() failed");
	pthread_exit (NULL);
  }
 
  
  #ifndef WIN32
  /* 
  code = post_connection_check (ssl, c[0]);
  if (code != X509_V_OK)
  {
	fprintf (stderr, "-Error: peer certificate: %s\n", X509_verify_cert_error_string (code));
	  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "receive_sync_file(): Err[004'] post_connection_check() failed");
	  pthread_exit (NULL);
  }
  */
  #endif
  for (i = 0; c[i]; i++)
	crv_free(c[i]); 

  fd = crv_fopen(host_port, "w");
  if (fd == NULL) {
	fclose(fd); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_sync_file(): Err[005] crv_fopen() failed");
	pthread_exit (NULL);
  }
  
  /* Build command -> SYNC#version */
  (void)crv_strncpy(command, "SYNC#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  
  code = SSL_write (ssl, command, (int)strlen(command));
  if ( code <= 0) {
	fclose(fd); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_sync_file(): Err[006] SSL_write() failed.");
	pthread_exit(NULL);
  }

  code = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[code] = '\0';
  if(!strncmp (buf, "SYNC_ACK", strlen("SYNC_ACK")))
  {
	for (;;)
	{
		code = SSL_read (ssl, buf, sizeof (buf));
		switch (SSL_get_error (ssl, code))
        {
		  case SSL_ERROR_NONE:
		  case SSL_ERROR_ZERO_RETURN:
		  case SSL_ERROR_WANT_READ:
		  case SSL_ERROR_WANT_WRITE:
			break;
		  default:
			fclose(fd); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
			fprintf(stderr, "0)Can't receive LISTING\n");
			pthread_exit (NULL);
		}

	  if (!strncmp (buf, "SYNC_END", strlen("SYNC_END"))) {
	    break;
	  }
	  code = fwrite (&buf, (size_t) code, 1, fd);
	  if (code <= 0) {
		fclose(fd); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	    fprintf(stderr, "1)Can't write LISTING file\n");
		pthread_exit (NULL);
	  }
	}
  }

  fclose(fd);
  close (sd);
  SSL_free (ssl);
  SSL_CTX_free (ctx);

  pthread_exit (NULL);
}

/* Files Table */
static int Files_callback(void *db, int argc, char **argv, char **azColName){
  int i;
	char *req = NULL;
  char *Sha1 = NULL;
	char *Name = NULL;
	char *Size = NULL;
	char *Date = NULL;
	sqlite3 *db_result = NULL;
  db_result = db;
	char *zErrMsg = 0;
  int rc;
	
	for(i=0; i<argc; i++){
		if(!crv_strncmp(azColName[i], "Sha1"))
			Sha1 = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Name"))
			Name = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Size"))
			Size = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Date"))
			Date = crv_strdup(argv[i]);
  }

  req = sqlite3_mprintf("INSERT into Files (Sha1, Name, Size, Date) values ('%q', '%q', '%q', '%q');", Sha1, Name, Size, Date);
	rc = sqlite3_exec(db_result, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	if(Sha1 != NULL)
		crv_free(Sha1);
	if(Name != NULL)
		crv_free(Name);
	if(Size != NULL)
		crv_free(Size);
	if(Date != NULL)
		crv_free(Date);
	return 0;
}


/* Files Categories */
static int Categories_callback(void *db, int argc, char **argv, char **azColName){
  int i;
	char *req = NULL;
  char *Sha1 = NULL;
	char *Cat = NULL;
	sqlite3 *db_result = NULL;
  db_result = db;
	char *zErrMsg = 0;
  int rc;
	
	for(i=0; i<argc; i++){
		if(!crv_strncmp(azColName[i], "Sha1"))
			Sha1 = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Cat"))
			Cat = crv_strdup(argv[i]);
  }

	req = sqlite3_mprintf("INSERT into Categories (Sha1, Cat) values ('%q', '%q');", Sha1, Cat); 
	rc = sqlite3_exec(db_result, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);
	if(Sha1 != NULL)
		crv_free(Sha1);
	if(Cat != NULL)
		crv_free(Cat);
	return 0;
}

/* Files Descr */
static int Descr_callback(void *db, int argc, char **argv, char **azColName){
  int i;
	char *req = NULL;
  char *Sha1 = NULL;
	char *Descr = NULL;
	char *Date = NULL;
	char *Pseudo = NULL;
	sqlite3 *db_result = NULL;
  db_result = db;
	char *zErrMsg = 0;
  int rc;
	
	for(i=0; i<argc; i++){
		if(!crv_strncmp(azColName[i], "Sha1"))
			Sha1 = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Descr"))
			Descr = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Pseudo"))
			Pseudo = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Date"))
			Date = crv_strdup(argv[i]);
  }

	req = sqlite3_mprintf("INSERT into Descr (Sha1,Descr,Pseudo,Date) values ('%q', '%q', '%q', '%q');", Sha1, Descr, Pseudo, Date);
	rc = sqlite3_exec(db_result, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	if(Sha1 != NULL)
		crv_free(Sha1);
	if(Descr != NULL)
		crv_free(Descr);
	if(Date != NULL)
		crv_free(Date);
	if(Pseudo != NULL)
		crv_free(Pseudo);
	return 0;
}

/* Files Grp_Sha1 */
static int Grp_Sha1_callback(void *db, int argc, char **argv, char **azColName){
  int i;
	char *req = NULL;
  char *Sha1 = NULL;
	char *groupname = NULL;
	sqlite3 *db_result = NULL;
  db_result = db;
	char *zErrMsg = 0;
  int rc;
	
	for(i=0; i<argc; i++){
		if(!crv_strncmp(azColName[i], "Sha1"))
			Sha1 = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "groupname"))
			groupname = crv_strdup(argv[i]);
  }

	req = sqlite3_mprintf("INSERT into Grp_Sha1 (Sha1, groupname) values ('%q', '%q');", Sha1, groupname);
	rc = sqlite3_exec(db_result, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);
	if(Sha1 != NULL)
		crv_free(Sha1);
	if(groupname != NULL)
		crv_free(groupname);
	return 0;
}


/* Server Table */
static int Server_callback(void *db, int argc, char **argv, char **azColName){
  int i;
	char *req = NULL;
  char *Sha1 = NULL;
	char *Servername = NULL;
	char *Port = NULL;
	char *bandwidth = NULL;
	sqlite3 *db_result = NULL;
  db_result = db;
	char *zErrMsg = 0;
  int rc;
	
	for(i=0; i<argc; i++){
		if(!crv_strncmp(azColName[i], "Sha1"))
			Sha1 = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Servername"))
			Servername = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "Port"))
			Port = crv_strdup(argv[i]);
		if(!crv_strncmp(azColName[i], "bandwidth"))
			bandwidth = crv_strdup(argv[i]);
  }

  req = sqlite3_mprintf("INSERT into Server (Sha1, Servername, Port, bandwidth) values ('%q', '%q', '%q', '%q');", Sha1, Servername, Port, bandwidth);
	rc = sqlite3_exec(db_result, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);
	if(Sha1 != NULL)
		crv_free(Sha1);
	if(Servername != NULL)
		crv_free(Servername);
	if(Port != NULL)
		crv_free(Port);
	if(bandwidth != NULL)
		crv_free(bandwidth);
	return 0;
}





static int sql_fusion(const char *dbname, sqlite3 *main_db)
{
	printf("Fusion de la base %s\n", dbname);
	int rc;
  sqlite3 *sdb,*ddb;
  sqlite3_backup *sql3b;
	char *zErrMsg = 0;
	//ddb = main_db;
		
		rc = sqlite3_open( dbname,&sdb);
    if(rc != SQLITE_OK){
        printf("%s\n",sqlite3_errmsg(ddb));
        return 0;
    }
	
		/* Copy Tables Files */
		rc = sqlite3_exec(sdb, "SELECT * FROM Files;", Files_callback, (void *)main_db, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}

		/* Copy Tables Categories */
		rc = sqlite3_exec(sdb, "SELECT * FROM Categories;", Categories_callback, (void *)main_db, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}

		/* Copy Tables Descr */
		rc = sqlite3_exec(sdb, "SELECT * FROM Descr;", Descr_callback, (void *)main_db, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}

		/* Copy Tables Grp_Sha1 */
		rc = sqlite3_exec(sdb, "SELECT * FROM Grp_Sha1;", Grp_Sha1_callback, (void *)main_db, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}

		/* Copy Tables Server */
		rc = sqlite3_exec(sdb, "SELECT * FROM Server;", Server_callback, (void *)main_db, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}

    sqlite3_close(sdb);
	return (0);
}

int Sync(void)
{
  FILE *fd = NULL;
  FILE *tmp_fd = NULL;
  char buf[SIZE];
  int i = 0;
  int j = 0;
	int flag = -1;
  DIR *dirp;
  struct dirent *f;
  struct stat sd;
  THREAD_TYPE id[10];
	sqlite3 *main_db;
  char *zErrMsg = 0;
  int rc;
 
  /* Clean listing directory */
	i = crv_remove_files_in_directory (options.listing_directory);
  if (i == -1) {
		fprintf( stderr, "%s\n", "Sync(): Err[001] clean listing directory failed");
		return (-1);
  }
	

  /* Get server:port from serveurliste file */
  fd = crv_fopen("serveur_liste", "r");
  if (fd == NULL) {
		fprintf( stderr, "%s%s%s\n", "Sync(): Err[002] Can't open ", options.path, "/serveur_liste");
		return (-1);
  }
  
  /* Read and annalyse each line */
  while (fgets(buf, sizeof(buf), fd) != NULL)
  {
    buf[strcspn(buf, "\n")] = '\0';
		/* Send SYNC command on serverS */

		THREAD_CREATE (id[i], (void *) receive_sync_file, buf);	
		sleep(1);
		i++;
  }
  if (!feof(fd)) {
    fprintf( stderr,"%s\n", "Sync(): Err[003] read serveur_liste failed");
    return (-1);
  }
  fclose(fd);
  
  j = i;
  for (i = 0; i < j; i++) {
		/*printf("DEBUG: thread(%d) de retour chez maman\n", i);*/
		pthread_join (id[i], NULL);
  }

	
	/* create only one listing file from all */
  i = crv_chdir (options.listing_directory);
  if (i == -1) {
		fprintf( stderr, "%s\n", "Sync(): Err[004] crc_chdir() listing directory failed");
		return (-1);
  }

	/* Create MAIN DATABASE */
	rc = sqlite3_open("../Database.db",&main_db);
  if(rc != SQLITE_OK){
		printf("%s\n",sqlite3_errmsg(main_db));
		return 0;
	}
	rc = sqlite3_exec(main_db, "CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_exec(main_db, "CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL, Pseudo TEXT NOT NULL, Date DATETIME DEFAULT '0000-00-00 00:00:00' NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_exec(main_db, "CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL, Date DATETIME DEFAULT '0000-00-00 00:00:00' NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_exec(main_db, "CREATE TABLE Grp_Sha1 (Sha1 TEXT NOT NULL, groupname TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_exec(main_db, "CREATE TABLE Server (Sha1 TEXT NOT NULL, Servername TEXT NOT NULL, Port NUMERIC NOT NULL, bandwidth NUMERIC NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}


	/* Put each DB-servername in main DB */
	dirp = opendir (".");
  if (dirp)
  {
		while ((f = readdir (dirp))!= NULL)
		{
			if (stat (f->d_name, &sd))
			{
				fprintf( stderr, "%s%s%s%s\n", "readdir(): Err[005] stat(", f->d_name, ") failed with error -> ", strerror(errno));
				(void)closedir (dirp);
				return (-1);
			}
			if ( strncmp(f->d_name, ".", strlen(f->d_name)) &&
					 strncmp(f->d_name, "..", strlen(f->d_name)) )
			{
				sql_fusion(f->d_name, main_db);
				fprintf(stdout, "-> %s\n", f->d_name);
			}
			/********/
		}
	}
  (void)closedir (dirp);
	sqlite3_close(main_db);


  i = crv_chdir(options.path);
  if (i == -1) {
		fprintf( stderr, "%s%s%s\n", "Sync(): crv_chdir(", options.path,") failed");
		exit(1);
  }
  return (0);
}
