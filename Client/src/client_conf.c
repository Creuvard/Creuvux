/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "client_conf.h"
#include "creuvard.h"

/* Client configuration options. */
ClientOptions options;

void initialize_client_options ( ClientOptions *options)
{
  char buf[SIZE];
  memset(options, 0, sizeof(*options));
  options->path = NULL;
  #ifndef WIN32
	(void)crv_strncpy(buf, (char *)crv_getenv ("HOME"), sizeof(buf));
	(void)crv_strncat(buf, "/creuvux/creuvux.conf", sizeof(buf));
  #else
	(void)strncpy(buf, "C:\\Program Files\\Creuvux\\creuvux.conf", sizeof(buf));
  #endif
  options->config = crv_strdup(buf);
  memset(buf, 0, sizeof(buf));
  options->debug = 1;
  options->sec = 1;
  options->passphrase = NULL;
  #ifndef WIN32
	(void)crv_strncpy(buf, (char *)crv_getenv ("HOME"), sizeof(buf));
	(void)crv_strncat(buf, "/creuvux/incoming", sizeof(buf));
  #else
	(void)strncpy(buf, "C:\\Program Files\\Creuvux\\incoming", sizeof(buf));
  #endif
  options->download_directory = crv_strdup(buf);
  memset(buf, 0, sizeof(SIZE)); 
  
  #ifndef WIN32
	(void)crv_strncpy(buf, (char *)crv_getenv ("HOME"), sizeof(buf));
	(void)crv_strncat(buf, "/creuvux/tmp", sizeof(buf));
  #else
	(void)strncpy(buf, "C:\\Program Files\\Creuvux\\tmp", sizeof(buf));
  #endif
  options->tmp_directory = crv_strdup(buf);
  memset(buf, 0, sizeof(SIZE));
  
  #ifndef WIN32
	(void)crv_strncpy(buf, (char *)crv_getenv ("HOME"), sizeof(buf));
	(void)crv_strncat(buf, "/creuvux/listing", sizeof(buf));
  #else
	(void)strncpy(buf, "C:\\Program Files\\Creuvux\\listing", sizeof(buf));
  #endif
  options->listing_directory = crv_strdup(buf);
  memset(buf, 0, sizeof(buf));
  options->address_family = AF_INET;
  options->srv = NULL;

  options->gui = 0;
  options->gui_addr = NULL;
  options->gui_port = -1;
  options->term_exec = NULL;
  options->crv_exec = NULL;
	options->gtk_info = NULL;
}


/*
 * read_client_conf(const char *filename)
 * -> Read configuration file and set options.
 */
int read_client_conf(const char *filename)
{
  FILE *fd = NULL;
  char buf[SIZE];
  char buffer[SIZE];
  int nb_sep = 0;
  int i;
  char **param = NULL;
  
  fd = crv_fopen(filename, "r");
  if (fd == NULL) {
	fprintf( stderr, "%s\n", "read_client_conf(): Err[001] crv_fopen() failed");
	return (-1);
  }
  
  while (fgets(buf, sizeof(buf), fd) != NULL)
  {
	buf[strcspn(buf, "\n")] = '\0';
	/* Check if first charactere on each line
       * is not: space
	   *		 # (comment)
       *		  \n 
       * and string =" is present
       */
	if ((buf[0] != '\n') && (buf[0] != '#') && (buf[0] != ' ') && strstr (buf, "=\"")) {
		for (i = 0; buf[i]; i++) 
		{
		  if (buf[i] == '"')
			nb_sep++;
	    }

	  if (nb_sep != 2) {
	    fprintf (stderr, "%s%s\n", " A \" is missing \n line: ", buf);
	    break;
	  }
	  
	  /* cut buf in 2 string. */
	  param = crv_cut (buf, "\"");

	  /* For fisrt string we replace the "=" of "CREUVUX_OPT=" with "CREUVUX_OPT\0" */
	  for (i = 0; param[0][i]; i++)
	    {
	      if (param[0][i] == '=')
		param[0][i] = '\0';
	    }

	  if (!strncmp( "CREUVUX_PATH", param[0], strlen("CREUVUX_PATH"))) {
		/*crv_free( options.path);*/
		options.path = crv_strdup(param[1]);
		(void)crv_strncpy(buffer, options.path, sizeof(buffer));
		(void)crv_strncat(buffer, "listing/", sizeof(buffer));
		options.listing_directory = crv_strdup(buffer);
		memset(buffer, 0, sizeof(buffer));
		(void)crv_strncpy(buffer, options.path, sizeof(buffer));
		(void)crv_strncat(buffer, "tmp/", sizeof(buffer));
		options.tmp_directory = crv_strdup(buffer);
		memset(buffer, 0, sizeof(buffer));

	  }

	  if (!strncmp( "CREUVUX_SEC", param[0], strlen("CREUVUX_SEC")))
	  {
		if (!strncmp( "no", param[1], strlen("no")))
		  options.sec = -1;
		else if (!strncmp( "yes", param[1], strlen("yes")))
		  options.sec = 1;
		else {
		  fprintf(stderr, "%s\n", "Please check CREUVUX_SEC=\"yes|no\"");
		  break;
		}
	  }

	  if (!strncmp( "CREUVUX_DEBUG", param[0], strlen("CREUVUX_DEBUG")))
	  {
		if (!strncmp( "no", param[1], strlen("no")))
		  options.debug = -1;
		else if (!strncmp( "yes", param[1], strlen("yes")))
		  options.debug = 1;
		else {
		  fprintf(stderr, "%s\n", "Please check CREUVUX_DEBUG=\"yes|no\"");
		  break;
		}
	  }

	  if (!strncmp( "CREUVUX_PWD", param[0], strlen("CREUVUX_PWD")))
		options.passphrase = crv_strdup(param[1]);

	  if (!strncmp( "CREUVUX_IPv", param[0], strlen("CREUVUX_IPv")))
	  {
		if (!strncmp( "4", param[1], strlen("4")))
		  options.address_family = AF_INET;
		else if (!strncmp( "6", param[1], strlen("6")))
		  options.address_family = AF_INET6;
		else {
		  fprintf(stderr, "%s\n", "Please check CREUVUX_IPv=\"4|6\"");
		  break;
		}
	  }
	  
	  if (!strncmp( "CREUVUX_DATA", param[0], strlen("CREUVUX_DATA")))
		options.download_directory = crv_strdup(param[1]);

	  if (!strncmp( "CREUVUX_SRV", param[0], strlen("CREUVUX_SRV")))
		options.srv = crv_strdup(param[1]);

		if (!crv_strncmp( "CREUVUX_COMPRESSION", param[0]))
	  {
			if (!strncmp( "no", param[1], strlen("no")))
				options.compression = -1;
			else if (!strncmp( "yes", param[1], strlen("yes")))
				options.compression = 1;
			else {
				fprintf(stderr, "%s\n", "Please check CREUVUX_COMPRESSION=\"yes|no\"");
				break;
			}
	  }
	  
	  nb_sep = 0;
	  for (i = 0; param[i]; i++)
	    crv_free(param[i]);
	  crv_free(param);
	}
  }
  if (!feof(fd)) {
	fclose(fd);
	return (-1);
  }
  fclose(fd);

	if (options.gui == 1) {
		char Gtk[SIZE];
		(void)crv_strncpy(Gtk, options.path, sizeof(Gtk));
		(void)crv_strncat(Gtk, "Gtk_info/", sizeof(Gtk));
		options.gtk_info = crv_strdup(Gtk);
		memset(Gtk, 0, sizeof(Gtk));
	}

  return (0);
}



void Free_options(void)
{
	if (options.config != NULL)
    crv_free(options.config);

  if (options.path != NULL)
    crv_free(options.path);

  if (options.passphrase != NULL)
    crv_free(options.passphrase);

  if (options.download_directory != NULL)
    crv_free(options.download_directory);

  if (options.listing_directory != NULL)
    crv_free(options.listing_directory);

  if (options.tmp_directory != NULL)
    crv_free(options.tmp_directory);

  if (options.srv != NULL)
    crv_free(options.srv);

  if (options.gui_addr != NULL)
    crv_free(options.gui_addr);

  if (options.term_exec != NULL)
    crv_free(options.term_exec);

  if (options.crv_exec != NULL)
    crv_free(options.crv_exec);

	if (options.gtk_info != NULL)
		crv_free( options.gtk_info);

}
