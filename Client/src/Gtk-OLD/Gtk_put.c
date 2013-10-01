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

/*
 * XML stuff
 */
#include <libxml/tree.h>
#include <libxml/parserInternals.h>
#include <libxml/xpath.h>



#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "xml_listing.h"
#include "SSL_sendfile.h"


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

int Gtk_Put(const char *filename, const char *file_descrip, const char *server, const char *Sha1)
{
  SSL_CTX* ctx = NULL;
  SSL*     ssl = NULL;
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
	int descr_fd = 0;
  errno = 0;

  /* ETA */
  struct timeval tv1, tv2;
  int sec1;
  
  
  init_OpenSSL ();			  
  seed_prng ();
	
  size = crv_du (filename);
  if (size == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  snprintf(size_c, sizeof(size), "%lld", size);

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
  (void)crv_strncat(command, Sha1, sizeof(command));
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
		code = SSL_sendfile(ssl, Sha1, filename, (off_t)0, size);	
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

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);


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

	fprintf(stderr, "File: '%s'\n", file_descrip);
	size = -1;
	size = crv_du (file_descrip);
	fprintf(stderr, "Size: '%lld'\n", size);
  if (size == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  memset(size_c, 0, sizeof(size_c));
  snprintf(size_c, sizeof(size), "%lld", size);

	sha1 = crv_sha1(file_descrip);
  
  /* Build command -> GET#version#sha1#begin#end */
  (void)crv_strncpy(command, "COMMENT#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, "0", sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
	fprintf(stderr, "CMD:'%s'\n", command);
  
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
		code = SSL_sendfile(ssl, sha1, file_descrip, (off_t)0, size);	
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

	code = unlink((const char *)file_descrip);
  if (code == -1) {
		fprintf(stderr, "%s%s\n", 
			"Put(): Err[010] unlink() failed with error -> ",
			strerror(errno));
		return (-1);
  }

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);crv_free(sha1);
  return (0);

}
