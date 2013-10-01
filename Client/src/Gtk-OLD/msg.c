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
/*
 * XML stuff
 */
#include <libxml/tree.h>
#include <libxml/parserInternals.h>
#include <libxml/xpath.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>


#include <pthread.h>

#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "xml_listing.h"
#include "get.h"
#include "msg.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

char *sha1;  /* file's sha1 fot wanted ID*/
char *server;/* Server for wanted ID */
int port;    /* Port for wanted ID */


/* Get Info server */
static void get_server_info(const char *ident)
{
	char *path = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	long lval;
	char *ep;

	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, ident, sizeof(xpath));
	(void)crv_strncat(xpath, "']/server" , sizeof(xpath));
	path = crv_strdup(xpath);

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
			node = xmlobject->nodesetval->nodeTab[j];
			xmlChar *Host = xmlGetProp(node, "host");
			if (Host != NULL) {
				server = crv_strdup(Host);
			}

			xmlChar *Port = xmlGetProp(node, "port");
			if (Port != NULL) {
				lval = strtol(Port, &ep, 10);
				if (Port[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s", Port, " is not a number");
					return;
				}
				port = lval;
			}
			xmlFree (Host);
			xmlFree (Port);
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}


/* Get sha1file */
static void get_sha1(const char *ident)
{
	char *path = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, ident, sizeof(xpath));
	(void)crv_strncat(xpath, "']" , sizeof(xpath));
	path = crv_strdup(xpath);

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
			node = xmlobject->nodesetval->nodeTab[j];
			xmlChar *Sha1 = xmlGetProp(node, "sha1");
			sha1 = crv_strdup(Sha1);
			xmlFree (Sha1);
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);	
}


int Msg(const char *id)
{
	FILE *fd = NULL;
	char buf[SIZE];
	char command[SIZE];
	int result = -1;
	off_t size;
	char size_c[SIZE];
	SSL_CTX* ctx = NULL;
  SSL*     ssl = NULL;
  int sd = -1;
	
	errno = 0;
	sha1 = NULL;  /* Sha1 for wanted ID */
	server = NULL;/* server for wanted ID */
	port = 0;			/* port for Wanted ID*/


	/* Get info about file */
	get_server_info(id);
	get_sha1(id);



	fd = crv_fopen(sha1, "w");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Msg(): Err[001] crv_fopen() failed");
		return (-1);
	}
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	fprintf(fd, "<comment><![CDATA[");
	fprintf(stdout, "%s", "\nEntrez votre reponse(':wq' pour quitter)\n");
	fprintf(stdout, "%s", ">");
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		if (!crv_strncmp(buf, ":wq"))
			break;
		fprintf(fd, "%s\n", buf);
		fprintf(stdout, "%s", ">");
	}
	fprintf(fd, "]]></comment>\n");
	fclose(fd);
	

	/*
	 * Build command -> GET#version#sha1#begin#end 
	 */
	size = crv_du (sha1);
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
		result = SSL_sendfile(ssl, NULL, sha1, (off_t)0, size);	
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

	result = unlink((const char *)sha1);
  if (result == -1) {
		fprintf(stderr, "%s%s\n", 
					"Msg(): Err[007] unlink() failed with error -> ",
					strerror(errno));
		return (-1);
  }
	
	return (0);
}

