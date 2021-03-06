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
#include <ctype.h>

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


#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "put.h"
#include "SSL_sendfile.h"
#include "list_grp.h"
#include "list_cat.h"


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;


static int many_files;

#define MULT_START     16384
#define MULT_INCREMENT 2
#define INIT_SIZE      1024

long memindx = MULT_START;
long memnext;


long addmem(char **buf, long size)
{


	memnext = (size > 0) ? size + memindx : INIT_SIZE;
	memindx = memindx * MULT_INCREMENT;
	char *tbuf = realloc(*buf, memnext);

	if (tbuf == NULL) {
		fprintf(stderr, "Can't allocate memory in addmem\n");
		return size;
	} else {
		*buf = tbuf;
		return memnext;
	}

}

/* If no server is selected by CREUVUX_UPLOAD, choice is asked */
char *give_server(void)
{
  char **Server = NULL;
	char *srv = NULL;
  FILE *fd = NULL;
  char buf[SIZE];
  int i = 0;

	Server = calloc(4096 * sizeof *Server, sizeof(char *));	

  /* Open server list */
  fd = crv_fopen("serveur_liste", "r");
  if (fd == NULL) {
		fprintf(stderr, "%s\n", "give_server(): Err[001] crv_fopen(\"serveur_liste\") failed");
		return (NULL);
  }

  /* Read and print each ligne */
  while (fgets(buf, sizeof(buf), fd) != NULL)
  {
		buf[strcspn(buf, "\n")] = '\0';
		fprintf(stdout, "[%d] %s\n", i, buf);
		Server[i] = crv_strdup( buf );
		i++;
  }
  if (!feof(fd)) {
		fprintf(stderr, "%s\n", "give_server(): Err[002] feof() failed");
		fclose(fd);
		return (NULL);
  }
	fclose(fd);
  i--; 
  memset(buf, 0, sizeof(buf));
  
  /* Ask choice */
  fprintf(stdout, "\nchoose number > "); fflush(stdout);
  if (fgets(buf, sizeof(buf), stdin) != NULL)
  {
		buf[strcspn(buf, "\n")] = '\0';
		char *ep;
		long lval;
		errno = 0;
		lval = strtol(buf, &ep, 10);
		if (buf[0] == '\0' || *ep != '\0') {
			fprintf(stderr, "%s%s%s\n", "give_server(): Err[003] ", buf, " is not number");
			return (NULL);
		}
		if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || (lval > i || lval < 0))
		{
			fprintf(stderr, "%s%s%s\n", "give_server(): Err[004] ", buf, " is out of range");
			return (NULL);
		}
		i = lval;
  }
  else {
		fprintf(stderr, "%s\n", "give_server(): Err[003] fgets() failed");
		return (NULL);
  }

	srv = crv_strdup(Server[i]);
	for (i=0; Server[i]; i++)
		crv_free(Server[i]);
	crv_free(Server);
	return (srv);
}

static int create_db (sqlite3 *db)
{
	char *zErrMsg = 0;
	int rc = -1;
	/*
	 * CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL);
	 * CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL);
	 * CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL);
	 * CREATE TABLE Grp_Sha1 (groupname TEXT NOT NULL, sha1 TEXT NOT NULL);
	 * CREATE TABLE Grp_User (groupname TEXT NOT NULL, username TEXT NOT NULL);
	 * CREATE TABLE Blob_Date (blob_title varchar(80), b blob);
	 */
	rc = sqlite3_exec(db, "CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Grp_Sha1 (groupname TEXT NOT NULL, sha1 TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Grp_User (groupname TEXT NOT NULL, username TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Blob_Data (blob_title varchar(80), b blob);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	return (0);
}

static int Put(sqlite3 *db, const char *filename, const char *server, const char *grp, const char *username, const char *genre)
{
  SSL_CTX* ctx = NULL;
  SSL*     ssl = NULL;
	FILE *fd = NULL;
  int sd = -1;
  int port = -1;
  char *ep;
  long lval;
  int code = -1;
  off_t size = 0;
  char command[SIZE];
  char buf[SIZE];
  char size_c [SIZE];
  char *dscr = NULL;
  char *sha1 = NULL;
  char **a = NULL;
  char *blob = NULL;
  char dbname[SIZE];
  char msg[4096];
  int descr_fd = 0;
  errno = 0;

  /* ETA */
  struct timeval tv1, tv2;
  int sec1;
 
	/* On ouvre le fichier pour voir si il existe */
  if ((fd = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Le fichier '%s' peut pas etre ouvert\n", filename);
      fprintf (stderr, "Verifiez les droits et son emplacements\n");
      exit (EXIT_FAILURE);
    }
	fclose (fd);
	
	char *real_name = NULL;
	real_name = basename((const char *)filename);
	if (real_name == NULL)
		return (NULL);
  
  size = crv_du (filename);
  if (size == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  snprintf(size_c, sizeof(size), "%lld", size);

	fprintf(stdout, "%s\n", "Determination de la signature numerique du fichier en cours ...");
	sha1 = crv_sha1(filename);

  /* Create info's file */
  (void)crv_strncpy (dbname, sha1, sizeof(dbname));
  (void)crv_strncat (dbname, ".db",  sizeof (dbname));

	if (many_files != 1) {
		int rep;
		fprintf (stdout, "Voulez-vous mettre des informations ? (o/n) :"); fflush(stdout);
		rep = fgetc (stdin);
		rep = toupper (rep);
		if (rep == 'O')
		{
			(void)crv_strncpy(msg, "", sizeof(msg));
			fprintf(stdout, "%s", "\nEntrez les informations du fichier (':wq' pour quitter)\n");
			fprintf(stdout, "%s", ">");
			memset(buf, 0, sizeof(buf));
			while (fgets(buf, sizeof(buf), stdin) != NULL) {
				if (!crv_strncmp(buf, ":wq\n"))
					break;
				(void)crv_strncat(msg, buf, sizeof(msg));
				fprintf(stdout, "%s", ">");
			}
		}
	}

	{
		int rep1;
		rep1 = -1;
		fprintf (stdout, "Voulez-vous mettre un Thumbnail ? (o/n) :"); fflush(stdout);
		rep1 = fgetc (stdin);
		rep1 = toupper (rep1);
		if (rep1 == 'O')
		{
			fprintf(stdout, "%s", "\nEntrez le chemin du fichier ('Enter' pour valider)\n");
			fprintf(stdout, "%s", ">");
			memset(buf, 0, sizeof(buf));
			while (fgets(buf, sizeof(buf), stdin) != NULL) {
				buf[strcspn(buf, "\n")] = '\0';
				if (strlen(buf) > 1)
					break;
			}
			blob = crv_strdup(buf);
		}

	}

	size = crv_du (filename);
  if (size == -1) {
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  memset(size_c, 0, sizeof(size_c));
  snprintf(size_c, sizeof(size_c), "%lld", size);
	/*
	fprintf (stdout, "DEBUG: DB Name='%s'\n", dbname);
	fprintf (stdout, "DEBUG: Sha1='%s'\n", sha1); 
  	fprintf (stdout, "DEBUG: Titre='%s'\n", real_name);
	fprintf (stdout, "DEBUG: Size='%s'\n", size_c);
	fprintf (stdout, "DEBUG: Pseudo='%s'\n", username);
	fprintf (stdout, "DEBUG: Message=\n`-> %s\n", msg);
	*/
	/*
	 * Database for file creation
	 */
	char *zErrMsg = 0;
	int rc = -1;
	char *req = NULL;
	sqlite3 *db_put;
	/* Open MAIN Database */
	rc = sqlite3_open( dbname, &db_put);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db_put));
		sqlite3_close(db_put);
		return (-1);
	}
	
	/* Create Tables */
	if (create_db (db_put) == -1) {
		fprintf(stderr, "%s\n", "Put(): Database creation Failed");
		return (-1);
	}

	/* Fill Files tables*/
	/* CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL); */
	req = sqlite3_mprintf("INSERT into Files (Sha1, Name, Size) values ('%q', '%q', '%q')", sha1, real_name, size_c);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL); */
	req = sqlite3_mprintf("INSERT into Categories (Sha1, Cat) values ('%q', '%q')", sha1, genre);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL); */
	if (strlen(msg) > 0) {
		req = sqlite3_mprintf("INSERT into Descr (Sha1, Descr) values ('%q', '%q')", sha1, msg);
	} else
		req = sqlite3_mprintf("INSERT into Descr (Sha1, Descr) values ('%q', '')", sha1);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* CREATE TABLE Grp_Sha1 (groupname TEXT NOT NULL, sha1 TEXT NOT NULL); */
	req = sqlite3_mprintf("INSERT into Grp_Sha1 (groupname, sha1) values ('%q', '%q')", grp, sha1);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	/* CREATE TABLE Grp_User (groupname TEXT NOT NULL, username TEXT NOT NULL); */
	if (username != NULL) {
		req = sqlite3_mprintf("INSERT into Grp_User (groupname, username) values ('%q', '%q')", grp, username);
		rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	/* CREATE TABLE Blob_Data (blob_title varchar(80), b blob); */
	/* insert into blobtest (des,b) values ('A test file: test.png',?); */
	if (blob != NULL) {
		int fd, n, i;
		long buflen = 0, totread = 0;
		char *bbuf = NULL, *pbuf = NULL;
		sqlite3_stmt *plineInfo = 0;

		if ((fd = open( blob, O_RDWR | O_CREAT, 0600)) == -1) {
			fprintf(stderr, "Can't open Blob: %s\n", strerror(errno));
			return 1;
		}
		while (buflen - totread - 1 < 1024)
		buflen = addmem(&bbuf, buflen);
		pbuf = bbuf;
		totread = 0;
		while ((n = read(fd, pbuf, 1024)) > 0) {
			totread += n;
			pbuf[n] = '\0';	// This is for printing test
			while (buflen - totread - 1 < 1024)
				buflen = addmem(&bbuf, buflen);
			pbuf = &bbuf[totread];
		}
		close(fd);
		req = sqlite3_mprintf("INSERT into Blob_Data (blob_title, b) values ('%q', ?)", "blob");
		rc = sqlite3_prepare(db_put, req, -1, &plineInfo, 0);
		if (rc == SQLITE_OK && plineInfo != NULL) {
			//fprintf(stderr, "SQLITE_OK\n");
			sqlite3_bind_blob(plineInfo, 1, bbuf, totread, free);
			sqlite3_step(plineInfo);
			rc = sqlite3_finalize(plineInfo);
		}	
		
	}
	sqlite3_close(db_put);

	/* Network zone */
	init_OpenSSL ();			  
  seed_prng ();
	a = crv_cut(server, ":");
  
	lval = strtol(a[1], &ep, 10);
  if (a[1][0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s\n", "Put(): Err[001] port is not a number");
		return (-1);
  }

  if ((errno == ERANGE && (lval == LONG_MAX
	  || lval == LONG_MIN)) ||
	  (lval > 65535 || lval < 0))
  {
		fprintf(stderr, "%s\n", "Put(): Err[002] port is out of range");
		return (-1);
  }
  port = lval;
  sd = crv_client_create( port, a[0], options.address_family);
  
  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "Put() Err[003] Create new ssl failed");
		return(-1);
  }

  /* connect the SSL object with a file descriptor */
  code = SSL_set_fd (ssl, sd);
  if ( code == 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "Put() Err[004] Put SSL on socket failed \n");
		return(-1);
  }

  code = SSL_connect (ssl);
  if (code == 0)	{
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[005] SSL_connect() failed");
		return(-1);
  } else
  if (code == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[006] SSL_connect() failed");
		return(-1);
  }

  /* Build command -> GET#version#sha1#begin#end */
  (void)crv_strncpy(command, "PUT#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, "0", sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
  
  /* Time initialisation */
  gettimeofday (&tv1, NULL);

  code = SSL_write (ssl, command, (int)strlen(command));
  if ( code <= 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_get_file(): Err[011] SSL_write() failed.");
	return(-1);
  }

  code = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[code] = '\0';

  if(!crv_strncmp (buf, "PUT_ACK"))
  {
		fprintf(stdout, "\n\n");
		code = SSL_sendfile(ssl, sha1, filename, (off_t)0, size);	
		if (code == -1) {
			close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
			fprintf(stderr, "%s\n", "Put() Err[012] SSL_sendfile() failed");
			return(-1);
		}
  }
  
  code = SSL_write (ssl, "PUT_END", (int)strlen("PUT_END"));
  if ( code <= 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[013] SSL_write() failed.");
		return(-1);
  }


  gettimeofday (&tv2, NULL);
  /* compute difference */
  sec1 = tv2.tv_sec - tv1.tv_sec;
  int min;
  float average;
  average = ((float) size / (float) sec1) / 1024;
  min = sec1 / 60;
  sec1 = sec1 - min * 60;
  fprintf (stdout,
	     "\n\nFile download in %d min  %d sec \nSpeed average -> %.f KBps\n\n",
	     min, sec1, average);

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);crv_free(sha1);


  /* Send file's decription */
  init_OpenSSL ();			  
  seed_prng ();
  sd = crv_client_create( port, a[0], options.address_family);
  
  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "Put() Err[003] Create new ssl failed");
	return(-1);
  }

  /* connect the SSL object with a file descriptor */
  code = SSL_set_fd (ssl, sd);
  if ( code == 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "Put() Err[004] Put SSL on socket failed \n");
	return(-1);
  }

  code = SSL_connect (ssl);
  if (code == 0)	{
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[005] SSL_connect() failed");
	return(-1);
  } else
  if (code == -1) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[006] SSL_connect() failed");
	return(-1);
  }

	size = crv_du (dbname);
  if (size == -1) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
	return(-1);
  }
  memset(size_c, 0, sizeof(size_c));
  snprintf(size_c, sizeof(size), "%lld", size);

	sha1 = crv_sha1(dbname);
  
  /* Build command -> GET#version#sha1#begin#end */
  (void)crv_strncpy(command, "COMMENT#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, "0", sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
  
  code = SSL_write (ssl, command, (int)strlen(command));
  if ( code <= 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_get_file(): Err[011] SSL_write() failed.");
	return(-1);
  }

  code = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[code] = '\0';

  if(!strncmp (buf, "PUT_ACK", strlen("PUT_ACK")))
  {
		code = SSL_sendfile(ssl, sha1, dbname, (off_t)0, size);	
		if (code == -1) {
			close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
			fprintf(stderr, "%s\n", "Put() Err[012] SSL_sendfile() failed");
			return(-1);
		}
  }
  
  code = SSL_write (ssl, "PUT_END", (int)strlen("PUT_END"));
  if ( code <= 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[013] SSL_write() failed.");
		return(-1);
  }


	fprintf(stdout, "%s", "Information about file is uploaded\n"); 

  /* Delete DB Meta Data */
  
  code = unlink((const char *)dbname);
  if (code == -1) {
		fprintf(stderr, "%s%s\n", 
			"Put(): Err[010] unlink() failed with error -> ",
			strerror(errno));
		return (-1);
  }
  

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);crv_free(sha1);
  return (0);

}


void pre_Put(sqlite3 *db, char **c)
{
	char *server = NULL;
	char *grp = NULL;
	char *username = NULL;
	char *cat = NULL;
	char line[SIZE];
	int i, j = 0;

	many_files = 0;

	server = give_server();

	if (server == NULL) {
		fprintf(stderr, "%s\n", "You must choose server id");
		return;
	}
	
	for (i = 1; c[i]; i++)
		j++;

	/* If several files is uploaded */
	if (j>1)
		many_files = 1;
	
	fprintf(stdout, "%s", "\n>Choix du groupe de destination.");
	fprintf(stdout, "%s", "\n>Seuls les menbres du groupe aurront acces a ce fichier.\n");
	
	/*
	 * Select Group
	 */
	grp = List_group_for_upload (db);
	fprintf (stdout, "Entrez le groupe de destination du groupe de fichier [all] : "); 
	fflush(stdout);
	
	/* If choice group is ok */
	if (grp != NULL) {
		fprintf(stdout, "%s\n", grp);
	}
	else
	{
		memset(line, 0, sizeof(line));
		if (fgets (line, sizeof(line), stdin) != NULL)
			line[strcspn(line, "\n")] = '\0';
		
		if (strlen(line) != 0)
			grp = crv_strdup(line);
		else
			grp = crv_strdup("all");
	}
	
	/*
	 * Select Username if need (grp = private)
	 */
  if (!crv_strncmp (grp, "private"))
  {
    fprintf (stdout, "Entrez le nom de l'utilisateur a qui s'adresse ce fichier/message [%s] : ", "CLIENT_PSEUDO"); fflush(stdout);
    if (fgets (line, sizeof(line), stdin) != NULL)
			line[strcspn(line, "\n")] = '\0';
	  else {
			fprintf(stderr, "Can't get user\n");
			return ;
	  }

    if (( strlen (line) == 0))
			username = crv_strdup ("CLIENT");
    else
			username = crv_strdup (line);
	  fprintf (stdout, "PSEUDO='%s'\n", username);
  }
	
	/*
	 * Select Categories
	 */
	fprintf(stdout, "\n>Choix de la categorie.\n");
	cat = List_cat_for_upload(db);	
	if (cat != NULL) {
		fprintf(stdout, "%s\n", cat);
	}
	else
	{
		fprintf (stdout, "Entrez le genre du fichier (SERIES, LIVECD ...) [%s] : ",  crv_file_type (c[1]));
		fflush(stdout);
		if(fgets (line, sizeof(line), stdin) != NULL)
			line[strcspn(line, "\n")] = '\0';
		else {
			fprintf(stderr, "Can't get categories\n");
			return ;
		}
  
		/* if genre is empty */
		if (((int) strlen (line) == 0))
			cat = crv_strdup (crv_file_type (c[1]));
		else
			cat = crv_strdup (line);
	}
  fprintf (stdout, "Categories='%s'\n", cat);

	for (i = 1; c[i]; i++) {
		if (server && c[i]) {
			printf("DEBUG: Put(%s, %s, %s, %s, %s);\n", c[i], server, grp, username, cat);
			Put(db, c[i], server, grp, username, cat);
		}
	}
	if (server != NULL)
		crv_free(server);
	if (grp != NULL)
		crv_free(grp);
	if (cat != NULL)
		crv_free(cat);
	if (username != NULL)
		crv_free(username);
	return;
}

void upload(char *filename)
{


	return;
}
