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


#include <pthread.h>

#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "xml_listing.h"


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
  crv_strncat (host_port, ":", sizeof(host_port));	/* add : behind server name */
  crv_strncat (host_port, c[1], sizeof(host_port));	/* add port behind : */
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


int Sync(void)
{
  FILE *fd = NULL;
  FILE *tmp_fd = NULL;
  char buf[SIZE];
  int i = 0;
  int j = 0;
  DIR *dirp;
  struct dirent *f;
  struct stat sd;
  THREAD_TYPE id[10];
  
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

  fd = crv_fopen( "listing.xml", "w");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Sync(): Err[004] crc_fopen() listing directory failed");
		return (-1);
  }
  fprintf( fd, "%s\n",
		"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
		"<database>");

  /* Read each file (expet 'listing.xml', '.' and '..') of directory and create an uniq */ 
  dirp = opendir (".");
  if (dirp)
  {
	while ((f = readdir (dirp))!= NULL)
	{
	  if (stat (f->d_name, &sd))
	  {
		fprintf( stderr, 
			"%s%s%s%s\n",  
			"readdir(): Err[005] stat(",
			f->d_name,
			") failed with error -> ", 
			strerror(errno));
		(void)closedir (dirp);
		(void)fclose(fd);
		return (-1);
	  }
	  if (strncmp(f->d_name, "listing.xml", strlen(f->d_name)) &&
		  strncmp(f->d_name, ".", strlen(f->d_name)) &&
		  strncmp(f->d_name, "..", strlen(f->d_name)) )
	  /******/
	  {
		fprintf(stdout, "-> '%s' Done\n", f->d_name);
		
		tmp_fd = crv_fopen( f->d_name, "r");
		if (tmp_fd == NULL) {
		  fprintf( stderr, "%s\n", "Sync(): Err[005] crc_fopen() listing directory failed");
		  fclose(fd);
		  (void)closedir(dirp);
		  return (-1);
		}
		while (fgets(buf, sizeof(buf), tmp_fd) != NULL) {
		  buf[strcspn(buf, "\n")] = '\0';
		  if (!strstr(buf, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>") &&
			  !strstr(buf, "<database>") &&
			  !strstr(buf, "</database>"))
		  {
			fprintf(fd, "%s\n", buf); 
		  }
		  
		}
		if (!feof(tmp_fd)) {
		  fclose(fd);
		  (void)closedir(dirp);
		  fprintf( stderr, "%s\n", "Sync(): Err[006] feof()failed");
		  return (-1);
		}
		fclose(tmp_fd);
		
	  }
	  /********/
	}
  }
  fprintf( fd, "%s\n", "</database>");
  fclose(fd);
  (void)closedir (dirp);
  memset(buf, 0, sizeof(buf));

  xsltransform ( "listing.xml", "plop.xml", 0);
  
  //xsltransform ( "plop_01.xml", "plop.xml", 1);
	i = unlink("listing.xml");
  if (i == -1) {
	fprintf( stderr, "%s%s\n", "Sync(): unlink(\"listing.xml\") failed with error -> ", strerror(errno));
	exit(1);
  }

  
  /*     °
   *    / \
   *   / | \  Ca a juste but de rajouter   le "xml-stylesheet"
   *  /  !  \  au fichier listing.xml.
   * /_______\
   *
   */

  fd = crv_fopen( "listing.xml", "w");
  if (fd == NULL) {
	fprintf( stderr, "%s\n", "Sync(): Err[004] crc_fopen() listing directory failed");
	return (-1);
  }

  tmp_fd = crv_fopen( "plop.xml", "r");
  if (tmp_fd == NULL) {
	fprintf( stderr, "%s\n", "Sync(): Err[005] crc_fopen() listing directory failed");
	return (-1);
  }

  while (fgets(buf, sizeof(buf), tmp_fd) != NULL) {
	buf[strcspn(buf, "\n")] = '\0';

	fprintf(fd, "%s\n", buf);
	if (strstr(buf, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>")) {
	  fprintf(fd, "%s\n", "<?xml-stylesheet type=\"text/xsl\" href=\"../web/listing.xsl\"?>"); 
	}
  }
  if (!feof(tmp_fd)) {
	fclose(fd);
	fclose(tmp_fd);
	fprintf( stderr, "%s\n", "Sync(): Err[006] feof()failed");
	return (-1);
  }
  
  fclose(tmp_fd);
  fclose(fd);
  
  /*
  i = rename ("plop.xml", "listing.xml");
  if (i == -1) {
	fprintf( stderr, "%s%s\n", "Sync(): rename(\"plop.xml\", \"listing.xml\") failed with error -> ", strerror(errno));
	exit(1);
  }
  */

  i = crv_chdir(options.path);
  if (i == -1) {
	fprintf( stderr, "%s%s%s\n", "Sync(): crv_chdir(", options.path,") failed");
	exit(1);
  }
  return (0);
}
