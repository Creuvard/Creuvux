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
#include "put.h"
#include "SSL_sendfile.h"


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

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
	fprintf(stderr, "DEBUG: Serveur %s\n", srv);
	for (i=0; Server[i]; i++)
		crv_free(Server[i]);
	crv_free(Server);
	return (srv);
}

char *
gen_description_file (const char *filename, const char *grp, const char *genre, const char *description)
{
	FILE *fd;
  FILE *fd1;
  char *real_name = NULL;
	char *sha1 = NULL;
  char Description[SIZE];
  char *dscr; /* returned char*  */
  char *tmp;

	sha1 = crv_sha1 (filename);

  /* On ouvre le fichier pour voir si il existe */
  if ((fd = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Le fichier '%s' peut pas etre ouvert\n", filename);
      fprintf (stderr, "Verifiez les droits et son emplacements\n");
      exit (EXIT_FAILURE);
    }
  tmp = crv_strdup(filename);

	real_name = basename((const char *)tmp);

	crv_free(tmp);
  if (real_name == NULL)
		return (NULL);
	/* Fichier dans lequel on mettra les infos a envoye au serveur */

  (void)crv_strncpy (Description, filename, sizeof(Description));
  (void)crv_strncat (Description, "-DSCR-.xml",  sizeof (Description));

  fd1 = crv_fopen (Description, "w");
  if (fd1  == NULL) {
	fprintf(stderr, "%s%s%s\n", "Open ", Description, " failed");
	fclose(fd);
	return (NULL);
  }

  fprintf (fd1, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
  fprintf(fd1, "<file name=\"%s\">\n", sha1);

  fprintf (fd1, "\t<title><![CDATA[%s]]></title>\n", real_name);

  fprintf (fd1, "\t<who group=\"%s\" name=\"\"/>\n", grp);
  fprintf (fd1, "\t<about>\n");
  if (genre == NULL)
		genre = crv_strdup(crv_file_type (filename));
  fprintf (fd1, "\t<genre>%s</genre>\n", genre);
	fprintf (fd1, "\t<comment><![CDATA[%s]]></comment>\n", description);
  fprintf (fd1, "\t</about>\n");
  fprintf (fd1, "</file>\n");
  fclose (fd1);
	fclose (fd);
	crv_free(sha1);
  dscr = crv_strdup(Description);
  
  return (dscr);
}


static char *
auto_make_description (const char *filename, const char *sha1, const char *grp, const char *genre)
{
  FILE *fd;
  FILE *fd1;
  char *real_name = NULL;
  char Description[SIZE];
  char *dscr; /* returned char*  */
  char *tmp;

  /* On ouvre le fichier pour voir si il existe */
  if ((fd = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Le fichier '%s' peut pas etre ouvert\n", filename);
      fprintf (stderr, "Verifiez les droits et son emplacements\n");
      exit (EXIT_FAILURE);
    }

  tmp = crv_strdup(filename);
	real_name = basename((const char *)tmp);
  crv_free(tmp);
	if (real_name == NULL)
		return (NULL);

  /* Fichier dans lequel on mettra les infos a envoye au serveur */

  (void)crv_strncpy (Description, filename, sizeof(Description));
  (void)crv_strncat (Description, "-DSCR-.xml",  sizeof (Description));

  fd1 = crv_fopen (Description, "w");
  if (fd1  == NULL) {
		fprintf(stderr, "%s%s%s\n", "Open ", Description, " failed");
		fclose(fd);
		return (NULL);
  }
	
  fprintf (fd1, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
  fprintf(fd1, "<file name=\"%s\">\n", sha1);

  fprintf (fd1, "\t<title><![CDATA[%s]]></title>\n", real_name);

  fprintf (fd1, "\t<who group=\"%s\" name=\"\"/>\n", grp);
  fprintf (fd1, "\t<about>\n");
  if (genre == NULL)
		genre = crv_strdup(crv_file_type (filename));
  fprintf (fd1, "\t<genre>%s</genre>\n", genre);
  fprintf (fd1, "\t</about>\n");
  fprintf (fd1, "</file>\n");
  fclose (fd1);


	/*
	 * TEST d'encodage ISO-8859-1
	 */
	/*
	if (NULL == (xmldoc = xmlParseFile (Description))) {
			fprintf(stderr, "%s", "create_about_file(): Err[001] xmlParseFile() failed\n");
			return(NULL);
		}
		if (NULL == (xmlroot = xmlDocGetRootElement (xmldoc))) {
			fprintf(stderr, "%s", "create_about_file(): Err[002] Doc is empty\n");
			xmlFreeDoc (xmldoc);
			return (NULL);
		}
		xmlSaveFileEnc (Description, xmldoc, "ISO-8859-1");
		xmlFreeDoc (xmldoc);
	*/

  fclose (fd);
  dscr = crv_strdup(Description);
  
  return (dscr);
}







static char *
make_description (const char *filename, const char *sha1)
{
  FILE *fd;
  FILE *fd1;
  char *real_name = NULL;
	char *result = (char *) NULL;
  char line[SIZE];
  char Description[SIZE];
  char *dscr; /* returned char*  */
  int rep;
  char *tmp;
	char *Grp = NULL;
	char *Cat = NULL;

  /* On ouvre le fichier pour voir si il existe */
  if ((fd = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Le fichier '%s' peut pas etre ouvert\n", filename);
      fprintf (stderr, "Verifiez les droits et son emplacements\n");
      exit (EXIT_FAILURE);
    }

	tmp = crv_strdup(filename);
	real_name = basename((const char *)tmp);
  crv_free(tmp);
	if (real_name == NULL)
		return (NULL);
  
  /* Fichier dans lequel on mettra les infos a envoye au serveur */

  (void)crv_strncpy (Description, filename, sizeof(Description));
  (void)crv_strncat (Description, "-DSCR-.xml",  sizeof (Description));

  fd1 = crv_fopen (Description, "w");
  if (fd1  == NULL) {
		fprintf(stderr, "%s%s%s\n", "Open ", Description, " failed");
		fclose(fd);
		return (NULL);
  }

  fprintf (fd1, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
  fprintf(fd1, "<file name=\"%s\">\n", sha1);

  fprintf (fd1, "\t<title><![CDATA[%s]]></title>\n", real_name);

  /* group wanted */
	fprintf(stdout, "%s", "\n>Choix du groupe de destination.\n");
	fprintf(stdout, "%s", ">Seuls les menbres du groupe aurront acces a ce fichier.\n");
	Grp = list_group_for_upload();
	fprintf (stdout, "Entrez le groupe de destination [all] : "); fflush(stdout);
  if (Grp != NULL) {
		result = crv_strdup(Grp);
		fprintf(stdout, "%s\n", result);
		crv_free(Grp);
	}
	else
	{
		if (fgets (line, sizeof(line), stdin) != NULL)
			line[strcspn(line, "\n")] = '\0';
		else {
			fclose(fd);
			fprintf(stderr, "Can't get group\n");
			return (NULL);
		}
  
		/* Si aucun group n'est specifie le group par defaut est "all" */
		if ((strlen (line) == 0))
			result = crv_strdup ("all");
		else
			result = crv_strdup (line);
  }

  fprintf (fd1, "\t<who group=\"%s\" ", result);
  /* > Si le groupe est "private" il s'agit d'un envoie a une personne precise
   * et dans ce cas la il faut le pseudo du type
   */
  if (!crv_strncmp (result, "private"))
  {
    fprintf (stdout, "Entrez le nom de l'utilisateur a qui s'adresse ce fichier/message [%s] : ", "CLIENT_PSEUDO"); fflush(stdout);
    if (fgets (line, sizeof(line), stdin) != NULL)
			line[strcspn(line, "\n")] = '\0';
	  else {
			fclose(fd);
			fprintf(stderr, "Can't get user\n");
			return (NULL);
	  }

    if (( strlen (line) == 0))
			result = crv_strdup ("CLIENT");
    else
			result = crv_strdup (line);
	  fprintf (fd1, "name=\"%s\" />\n", result);
  }
  else
    fprintf (fd1, "name=\"\"/>\n");

  fprintf (fd1, "\t<about>\n");
  crv_free(result);

  /* file type wanted */
	fprintf(stdout, "Choix de la categorie.\n");
	Cat = list_categorie_for_upload();
	fprintf (stdout, "Entrez le genre du fichier (SERIES, LIVECD ...) [%s] : ",
	   crv_file_type (filename));
	fflush(stdout);
	if (Cat != NULL) {
		result = crv_strdup(Cat);
		fprintf(stdout, "%s\n", result);
		crv_free(Cat);
	}
	else
	{
		if(fgets (line, sizeof(line), stdin) != NULL)
			line[strcspn(line, "\n")] = '\0';
		else {
			fclose(fd);
			fprintf(stderr, "Can't get genre\n");
			return (NULL);
		}
  
		/* if genre is empty */
		if (((int) strlen (line) == 0))
			result = crv_strdup (crv_file_type (filename));
		else
			result = crv_strdup (line);
	}
  fprintf (fd1, "\t<genre>%s</genre>\n", result);
  crv_free(result);


  /* Comment */
  
  fprintf (stdout, "Voulez-vous mettre un commentaire ? (o/n) :"); fflush(stdout);
  rep = fgetc (stdin);
  rep = toupper (rep);
  if (rep == 'O')
  {
      printf ("Balance tout man ( \":wq\" pour quitter!)\n\n\n>");
      fprintf (fd1, "\t<comment><![CDATA[");
	  while (fgets(line, sizeof(line), stdin) != NULL) {
		line[strcspn(line, "\n")] = '\0';
		
		if (!strncmp (line, ":wq", strlen (":wq")))
		{
		  fprintf (fd1, "\t]]></comment>\n");
		  break;
		}
		fprintf (fd1, "%s\n", line);
      }
  }
  
  fprintf (fd1, "\t</about>\n");
  fprintf (fd1, "</file>\n");
  fclose (fd1);
  fclose (fd);

  dscr = crv_strdup(Description);
  
  return (dscr);
}









int Put(const char *filename, const char *server, const char *grp, const char *genre)
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
	char Description[SIZE];
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

	fprintf(stdout, "%s\n", "Determination de la signature numerique du fichier en cours ...");
	sha1 = crv_sha1(filename);

  /* Create info's file */
  (void)crv_strncpy (Description, filename, sizeof(Description));
  (void)crv_strncat (Description, "-DSCR-.xml",  sizeof (Description));

	descr_fd = open(Description, O_RDONLY);
	
	if (descr_fd == -1)
	{
		if (grp == NULL)
			dscr = make_description (filename, sha1); 
		else
			dscr = auto_make_description (filename, sha1, grp, genre);
	} else {
		close(descr_fd);
		dscr = crv_strdup(Description);
	}

  if (dscr == NULL) {
	  fprintf(stderr, "%s\n", "Can't make file's descrition");
	  return (-1);
	}

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

	size = crv_du (dscr);
  if (size == -1) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
	return(-1);
  }
  memset(size_c, 0, sizeof(size_c));
  snprintf(size_c, sizeof(size), "%lld", size);

	sha1 = crv_sha1(dscr);
  
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
		code = SSL_sendfile(ssl, sha1, dscr, (off_t)0, size);	
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

	code = unlink((const char *)dscr);
  if (code == -1) {
		fprintf(stderr, "%s%s\n", 
			"Put(): Err[010] unlink() failed with error -> ",
			strerror(errno));
		return (-1);
  }

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);crv_free(sha1);
  return (0);

}
