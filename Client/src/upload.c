#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
/*
 * CURl stuff
 */
#include <curl/curl.h>

/*
 * XML stuff
 */
#include <libxml/tree.h>
#include <libxml/parserInternals.h>
#include <libxml/xpath.h>

/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "network.h"
#include "SSL_sendfile.h"

/*
 * CREUVUX Stuff
 */
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "upload.h"
#include "SSL_sendfile.h"
#include "list_grp.h"
#include "list_cat.h"


#define SIZE 1024


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;


static char *COOKIES;
static FILE *bodyfile;
static char **Categories = NULL;
static int nb_cat = 0;


/***************************************/
/* DEBUT DE L'UTILISATION DE L'API DOM */
/***************************************/
xmlDocPtr xmldoc;

/* Stock xml_download file in memory */
void download_dom_in (const char *file)
{
	xmldoc = NULL;

	if (NULL == (xmldoc = xmlParseFile (file))) {
		fprintf(stderr, "%s", "download_dom_in(): Err[001] xmlParseFile() failed\n");
		return;
	}
	
}

/* Drop xml_download memory */
void download_dom_out (void)
{
	xmlFreeDoc (xmldoc);
}

void list (void)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval ("/Categories/cat/text()", xml_context);


	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
	{
		nb_cat = 0;
		xmlNodePtr node;
		for (nb_cat = 0; nb_cat < xmlobject->nodesetval->nodeNr; nb_cat++)
		{
			node = xmlobject->nodesetval->nodeTab[nb_cat];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				Categories[nb_cat] = crv_strdup( node->content );
				printf("(%d) -> %s\n", nb_cat, node->content);
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);

}


size_t write_data(char *ptr, size_t size, size_t nmemb, void *stream)
{
	char *r = strstr(ptr, "PHPSESSID=");
	if ( r != NULL)
		COOKIES = strdup(r);
	return strlen(ptr);
}

static size_t printcat(char *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)bodyfile);
  return written;
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

int upload(const char *filename, const char *Category, const char *server)
{
  CURL *curl_handle;
	char *tmpfile = NULL;
	char *sha1 = NULL;
	char *basen = NULL;
	char *truename = NULL;
	char buf[SIZE];
	int i = -1;
	long lval = -1;
	char *ep = NULL;
	int port = -1;
	int sd = -1;
	int code = -1;
	SSL_CTX* ctx = NULL;
  SSL*     ssl = NULL;
	FILE *fd = NULL;
	off_t size = 0;
  char command[SIZE];
  char size_c [SIZE];
	char **a = NULL;
	/* ETA */
  struct timeval tv1, tv2;
  int sec1;

	fprintf(stderr, "Debug: fichier uploadé = %s\n", filename);
	fprintf(stdout, "%s\n", "Determination de la signature numerique du fichier en cours ...");
	sha1 = crv_sha1(filename);
	printf("Sha1 (%s) = %s\n", filename, sha1);

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

	size = crv_du (filename);
  if (size == -1) {
		SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  snprintf(size_c, sizeof(size), "%lld", size);


	basen = crv_strdup(filename);
	truename = basename(basen);
	printf("Categorie = %s\n", Category);
	printf("Titre: = %s\n", truename);
	printf("Sha1= %s\n", sha1);
	printf("Taille= %s\n", size_c);

	/* Build http post command  */
	(void)crv_strncpy(command, "http://localhost/crv2/control/get-clientc-file.php?filename=", sizeof(command));


  //(void)crv_strncpy(command, "filename=", sizeof(command));
  (void)crv_strncat(command, truename, sizeof(command));
	(void)crv_strncat(command, "&", sizeof(command));
  
	(void)crv_strncat(command, "sha1=", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "&", sizeof(command));

	(void)crv_strncat(command, "size=", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
  (void)crv_strncat(command, "&", sizeof(command));
  
	(void)crv_strncat(command, "cat=", sizeof(command));
  (void)crv_strncat(command, Category, sizeof(command));
	
	printf("HTTP POST -> %s\n", command);
	/*
	 * Send info on webapp
	 */
	/* init the curl session */
  curl_handle = curl_easy_init();

	/* set URL to get */
	//curl_easy_setopt(curl_handle, CURLOPT_URL, "http://creuvux.org/model/verifLogin.php");
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://localhost/crv2/model/verifLogin.php");
	 curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 0);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "login=Sylvain&password=16645169");
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "login=creuvard&password=1664");
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, stdout);

  /* get it! */
  curl_easy_perform(curl_handle);
  
	/* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

	curl_handle = curl_easy_init();

  /* set URL to get */

	//curl_easy_setopt(curl_handle, CURLOPT_URL, "http://localhost/crv2/control/get-clientc-file.php");
	curl_easy_setopt(curl_handle, CURLOPT_URL, command);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 0);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIE, COOKIES);
	//curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, command);



	char errbuf[CURL_ERROR_SIZE];
	CURLcode errnum;

  /* get it! */
	errnum  = curl_easy_perform(curl_handle);
  if (CURLE_OK != errnum)
	{
		fprintf(stderr, "%dn", errnum);
	}
	
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);
	
	/*
	 * Tempory end of fonction
	 * */
	sleep(1);

	return (0);
	/*
	 * Send file
	 */

	/* On ouvre le fichier pour voir si il existe */
  if ((fd = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Le fichier '%s' peut pas etre ouvert\n", filename);
      fprintf (stderr, "Verifiez les droits et son emplacements\n");
      exit (EXIT_FAILURE);
    }
	fclose (fd);

	size = crv_du (filename);
  if (size == -1) {
		SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  snprintf(size_c, sizeof(size), "%lld", size);



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



	/*
	 * end of sending zone
	 */
	for (i=0; Categories[i]; i++)
		if (Categories[i] != NULL)
			crv_free(Categories[i]);
	if (Categories != NULL)
		crv_free(Categories);
  return 0;
}

char *choice_cat(void)
{
  CURL *curl_handle;
	char *tmpfile = NULL;
	char buf[SIZE];
	int i = -1;
	long lval = -1;
	char *ep = NULL;
	int port = -1;
	int sd = -1;
	int code = -1;
	FILE *fd = NULL;
	off_t size = 0;
  char command[SIZE];
  char size_c [SIZE];
	char **a = NULL;
	/* ETA */
  struct timeval tv1, tv2;
  int sec1;



	Categories = calloc(4096 * sizeof *Categories, sizeof(char *));

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* set URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://creuvux.org/model/verifLogin.php");
	//curl_easy_setopt(curl_handle, CURLOPT_URL, "http://localhost/crv2/model/verifLogin.php");
	 curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 0);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "login=Sylvain&password=16645169");
	//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "login=creuvard&password=1664");
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, stdout);

  /* get it! */
  curl_easy_perform(curl_handle);
	
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

	/*
	 * Creation d'un fichier temporaire pour y mettre la page web 
	 * contenant la liste des catégories
	 */
	tmpfile = crv_mkstemp("tempo", "");

	/* GET CAT */
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://creuvux.org/control/srvapi_lscat.php");
	//curl_easy_setopt(curl_handle, CURLOPT_URL, "http://localhost/crv2/control/srvapi_lscat.php");
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIE, COOKIES);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, printcat);
	bodyfile = fopen(tmpfile,"w");
  if (bodyfile == NULL) {
    curl_easy_cleanup(curl_handle);
    return -1;
  }
	curl_easy_perform(curl_handle);
	curl_easy_cleanup(curl_handle);
	fclose(bodyfile);
	
	/*
	 * Ouvrir le fichier temoraire pour y extraire la liste des catégories
	 * et les stocker dans une liste/tableau
	 */
	download_dom_in (tmpfile);
	list();
	download_dom_out ();
	unlink(tmpfile);

	/*
	 * Choisir la catégorie souhaitée.
	 */

	/* Ask choice */
  fprintf(stdout, "\nchoose number (Enter pour creer une nouvelle catégorie)> "); fflush(stdout);
  if (fgets(buf, sizeof(buf), stdin) != NULL)
  {
		buf[strcspn(buf, "\n")] = '\0';
		lval = -1;
		errno = 0;
		lval = strtol(buf, &ep, 10);
		if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || (lval > nb_cat || lval < 0))
		{
			fprintf(stderr, "%s%s%s\n", "upload(): Err[004] ", buf, " is out of range");
			return (NULL);
		}
		i = lval;
		/*
		 * If input is not a number (like Enter), creuvux ask to create a
		 * new category.
		 */
		if (buf[0] == '\0' || *ep != '\0') {
			fprintf(stderr, "%s%s%s\n", "upload(): Err[003] ", buf, " is not number");
			i = -1;
		}
  }
  else {
		fprintf(stderr, "%s\n", "uplad(): Err[003] fgets() failed");
		return (NULL);
  }
	if (i == -1) {
		fprintf(stdout, "\nNom de la nouvelle catégorie)> "); fflush(stdout);
		if (fgets(buf, sizeof(buf), stdin) != NULL)
			buf[strcspn(buf, "\n")] = '\0';
		Categories[0] = crv_strdup(buf);
	} else	
		Categories[0] = crv_strdup(Categories[i]);
	
	return (Categories[0]);
}
