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

/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>


#include <pthread.h>

#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
//#include "get.h"
#include "msg.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

char *sha1;  /* file's sha1 fot wanted ID*/
char *server;/* Server for wanted ID */
int port;    /* Port for wanted ID */

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
		if (!crv_strncmp(azColName[i], "Servername"))
			server = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Sha1"))
			sha1 = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Port"))
			port = (int)atoi(argv[i]);
  }
  return 0;
}

int Msg(sqlite3 *db, const char *id, const char *comment)
{
	//FILE *fd = NULL;
	char buf[SIZE];
	char dbname[SIZE];
	char command[SIZE];
	int result = -1;
	off_t size;
	char size_c[SIZE];
	SSL_CTX* ctx = NULL;
  SSL*     ssl = NULL;
  int sd = -1;
	sqlite3 *db_msg;
	char *zErrMsg = 0;
	int rc = -1;
	char msg[4096];
	char *req = NULL;
	
	errno = 0;
	sha1 = NULL;  /* Sha1 for wanted ID */
	server = NULL;/* server for wanted ID */
	port = 0;			/* port for Wanted ID*/
	
	/*  Get info about file 
	SELECT DISTINCT Files.Sha1,Server.servername,Server.Port FROM Files,Server WHERE Files.rowid='6' AND Files.Sha1=Server.Sha1;
	*/
	req = sqlite3_mprintf("SELECT DISTINCT Files.Sha1,Server.servername,Server.Port FROM Files,Server WHERE Files.rowid='%q' AND Files.Sha1=Server.Sha1;", id);

	rc = sqlite3_exec(db, req, callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);
	
	//if (req != NULL)
	//	crv_free(req);
	req = NULL;

	(void)crv_strncpy(dbname, "msg_", sizeof(dbname));	
	(void)crv_strncat(dbname, id, sizeof(dbname));
	(void)crv_strncat(dbname, ".db", sizeof(dbname));

	/* DEBUG */
	
	fprintf(stderr, "dbname=%s\n", dbname);
	fprintf(stderr, "Server=%s\n", server);
	fprintf(stderr, "Port=%d\n", port);
	fprintf(stderr, "sha1=%s\n", sha1);

	

	/* Open Msg Database */
	rc = sqlite3_open( dbname, &db_msg);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db_msg));
		sqlite3_close(db_msg);
		return (-1);
	}
	rc = sqlite3_exec(db_msg, "CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	if (comment == NULL) {
		(void)crv_strncpy(msg, "", sizeof(msg));
		fprintf(stdout, "%s", "\nEntrez votre reponse(':wq' pour quitter)\n");
		fprintf(stdout, "%s", ">");
		while (fgets(buf, sizeof(buf), stdin) != NULL) {
			if (!crv_strncmp(buf, ":wq\n"))
				break;
			(void)crv_strncat(msg, buf, sizeof(msg));
			fprintf(stdout, "%s", ">");
		}
	}

	if (comment == NULL)
		req = sqlite3_mprintf("INSERT into Descr (Sha1, Descr) values ('%q', '%q')", sha1, msg);
	else
		req = sqlite3_mprintf("INSERT into Descr (Sha1, Descr) values ('%q', '%q')", sha1, comment);
	rc = sqlite3_exec(db_msg, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_close(db_msg);

	/*
	 * Build command -> GET#version#sha1#begin#end 
	 */
	size = crv_du (dbname);
  if (size == -1) {
		fprintf( stderr, "%s\n", "Put(): Err[002] crv_du() can't get size");
		return(-1);
  }
  snprintf(size_c, sizeof(size), "%lld", size);

  (void)crv_strncpy(command, "MSG_REPLY#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, "0", sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
	/*
	 * Connection.
	 */
	init_OpenSSL ();			  
  seed_prng ();
	sd = crv_client_create( port, server, options.address_family);

  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "Msg() Err[003] Create new ssl failed");
		return(-1);
  }

  /* connect the SSL object with a file descriptor */
  result = SSL_set_fd (ssl, sd);
  if ( result == 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "Msg() Err[004] Put SSL on socket failed \n");
		return(-1);
  }

  result = SSL_connect (ssl);
  if (result == 0)	{
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Msg(): Err[005] SSL_connect() failed");
		return(-1);
  } else
  if (result == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Msg(): Err[006] SSL_connect() failed");
		return(-1);
  }

	result = SSL_write (ssl, command, (int)strlen(command));
  if ( result <= 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[011] SSL_write() failed.");
		return(-1);
  }

  result = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[result] = '\0';

  if(!strncmp (buf, "PUT_ACK", strlen("PUT_ACK")))
  {
		fprintf(stdout, "\n\n");
		result = SSL_sendfile(ssl, sha1, dbname, (off_t)0, size);	
		if (result == -1) {
			close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
			fprintf(stderr, "%s\n", "Put() Err[012] SSL_sendfile() failed");
			return(-1);
		}
  }
  
  result = SSL_write (ssl, "PUT_END", (int)strlen("PUT_END"));
  if ( result <= 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[013] SSL_write() failed.");
		return(-1);
  }

	result = unlink((const char *)dbname);
  if (result == -1) {
		fprintf(stderr, "%s%s\n", 
					"Msg(): Err[007] unlink() failed with error -> ",
					strerror(errno));
		return (-1);
  }
	return (0);
}

