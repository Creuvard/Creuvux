/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 * This program is the creuvux daemon.  It listens for connections from clients,
 * and performs authentication, executes use commands, and forwards
 * information to/from the application to the user client over an encrypted
 * connection.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <pwd.h>
#include <libgen.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * SSL Stuff
 */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/*
 * SQLITE3 Stuff
 */

#include <sqlite3.h>



#include "creuvard.h"
#include "client_conf.h"
#include "help.h"
#include "sync.h"
#include "get.h"
#include "msg.h"
#include "find.h"
#include "list.h"
#include "info.h"
#include "list_grp.h"
#include "list_cat.h"
#include "upload.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;


/*
 * usage()
 */
static  void
usage (const char *str)
{
  if (str)
    printf ("\n%s\n", str);

  fprintf (stdout, "%s\n", "Usage : creuvuxd [-hv] [-f config_file] [-c command arg]" );
}

/*
 * Get passphrase
 */
static int
get_passwd (void)
{
  char *cr;
  int result;
  char *p;
  char *pwd;
  pwd = crv_malloc (SIZE);
  struct termios t;
  /* Desactive l'affichage des caracteres */
  tcgetattr (0, &t);
  t.c_lflag &= !ECHO;
  tcsetattr (0, TCSANOW, &t);
/* Lit le mot de passe */
  fgets (pwd, SIZE, stdin);
  if ((p = strchr (pwd, '\n')) == NULL)
    {
      fprintf (stderr, "Password line too long.\n");
      exit (1);
    }
/* Reactive l'affichage des caracteres */
  tcgetattr (0, &t);
  t.c_lflag |= ECHO;
  tcsetattr (0, TCSANOW, &t);
/* Supprime le caractÃ¯Â¿Â½re de fin de ligne
 * et tout ce qui suit s'il est prÃ¯Â¿Â½sent
 */
  if ((cr = strchr (pwd, '\n')))
    cr[0] = '\0';
  fprintf (stdout, "\n");
  
	options.passphrase = crv_strdup(pwd);
	free (pwd);
  return (result);
}


/* check if Arg is valid */
static int check_id(const char *Id_arg)
{
	int file_id = -1;
	char *ep;
	long lval;
	errno = 0;
	file_id = return_max_id ();
	
	lval = strtol(Id_arg, &ep, 10);
	if (Id_arg[0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s\n", "Argument is not a number");
		return (-1);
	}
	
	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
	(lval > file_id || lval < 0))
	{
		fprintf(stderr, "%s\n", "Argument seems to be bad");
		return (-1);
	}
	return (1);
}

/*
 * Main program for daemon.
 */
int main (int argc, char **argv)
{
  int opt;
  int result = -1;
  struct stat st; /* check options.chroot_directory */
  FILE *fd;		   /* Check *.pem files */
  char *command = NULL;
	char *cat = NULL;
	char *server = NULL;
  char buf[SIZE];
  char **c = NULL;
  int i = 0;
  char **arg = NULL; /* Use when creuvux is run with option from command line */
  int arg_inc = 0;
	int mode_script = -1; /* if mode_script = 1, script mode is enable */
	
	char *zErrMsg = 0;
	int rc = -1;

  
  /* Ensure that standart files descriptor 0, 1 and 2 are open or directed to /dev/null */
  crv_sanitise_stdfd();
  
  /* Initialize configuration options to their default values. */
  initialize_client_options(&options);

  while ((opt = getopt(argc, argv, "f:c:hv")) != -1)
  {
	switch(opt) {
	  case 'h': /* Print Help */
			fprintf(stdout, "%s\n", "Help:");
			usage (NULL);
			exit(EXIT_SUCCESS);
	  case 'v': /* Print version */
			fprintf (stderr, "%s%s\n", "creuvux version ", CREUVUX_VERSION);
			fprintf (stderr, "%s\n", "Before reporting bugs (creuvard@colerix.org) please check "
								 "the development CHANGELOG to make sure it hasn't already "
								 "been fixed in devel.");
			exit(1);
	  case 'f': /* Config File */
			crv_free(options.config);
			options.config = crv_strdup(optarg);
			break;
	  case 'c': /* Get command */
			mode_script = 1;
			if (NULL == (c = malloc (SIZE * sizeof *c)))
					exit(EXIT_FAILURE);
			for (i = 2; argv[i]; i++)
				c[i-2] = crv_strdup(argv[i]);

			for (i = 0; c[i]; i++)
				printf("-> %s\n", c[i]);
			break;
	  case '?':
	  default:
			usage(NULL);
			exit(EXIT_FAILURE);
		}
  }
  argc -= optind;
  argv += optind;

  /* Read configuration file and set options */
  result = read_client_conf (options.config);
  if (result == -1)
    return (-1);

	/* Check if passphrase isn't NULL */
	if (options.passphrase == NULL) {
		fprintf(stdout, "%s", "Enter your passphrase:");
		fflush(stdin);
		get_passwd ();
		fprintf(stdout, "%s", "\n");
	}
  
  /*
  if (optind < argc) {
	fprintf( stderr, "%s%s\n", "Extra argument ", argv[optind]);
	exit(1);
  }
  */

  /*
   * Check Configuration 
   */


  /* Check upload_directory */
  if ((stat(options.download_directory, &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
	fprintf( stderr, "Missing download directory: %s\n", options.download_directory);
	exit(1);
  }

  /* Check config directory */
  if ((stat(options.path, &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
	fprintf( stderr, "Option CREUVUX_PATH is missing: %s\n", options.path);
	exit(1);
  }

  result = crv_chdir(options.path);
  if (result == -1) {
	fprintf( stderr, "%s%s%s\n", "main(): crv_chdir(", options.path,") failed");
	exit(1);
  }


  /* Check existence of tmp directory */
  if ((stat( "./tmp", &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
	fprintf( stderr, "Directory %stmp/ is missing\n", options.tmp_directory);
	exit(1);
  }
  

  /* Check server.pem */
  fd = crv_fopen("client.pem", "r");
  if (fd == NULL) {
	fprintf( stderr, "%s\n", "Missing server.pem.");
	return (-1);
  }
  fclose(fd);
  
  /* Check rootcert.pem */
  fd = crv_fopen("rootcert.pem", "r");
  if (fd == NULL) {
	fprintf( stderr, "%s\n", "Missing rootcert.pem");
	return (-1);
  }
  fclose(fd);
  
  /* ignore SIGPIPE */
  signal (SIGPIPE, SIG_IGN);
	options.gui = 0;

  welcom();
  if (options.debug == 1)
  {
	fprintf(stderr, "%s\n", "#################");
	fprintf(stderr, "%s\n", "# MODE DEBUG ON #");
	fprintf(stderr, "%s\n", "#################");
	fprintf(stderr, "# | \n");
	fprintf(stderr, "# |-> CREUVUX_PATH  :'%s'\n", options.path);
	fprintf(stderr, "# |-> CREUVUX_SEC   :'%d'\n", options.sec);
	fprintf(stderr, "# |-> CREUVUX_PWD   :'%s'\n", options.passphrase);
	fprintf(stderr, "# |-> CREUVUX_DATA  :'%s'\n", options.download_directory);
	fprintf(stderr, "# |-> LISTING_DIR   :'%s'\n", options.listing_directory);
	fprintf(stderr, "# |-> TMP_DIR       :'%s'\n", options.tmp_directory);
	if ( options.address_family == AF_INET) 
		fprintf(stderr, "%s", "# |-> CREUVUX_ADDR  :'IPv4'\n");
	else if ( options.address_family == AF_INET6)
		fprintf(stderr, "%s", "# |-> CREUVUX_ADDR  :'IPv6'\n");
	if (options.compression == 1)
	fprintf(stderr, "%s", "# |-> CREUVUX_COMPRESSION  :'Yes'\n");
	else if (options.compression == -1)
	fprintf(stderr, "%s", "# |-> CREUVUX_COMPRESSION  :'No'\n");
	fprintf(stderr, "%s\n\n", "#################");
  }


	/*
	 * Deux fonctions:
	 * -> Envoie de fichiers
	 * -> Téléchargement de fichiers
	 */
	// Download
	/*
	fprintf(stdout, "File: %s\n", c[i]);
	char *sha1 = NULL;
	sha1 = crv_sha1(c[i]);
	printf("Sha1 (%s) = %s\n", c[i], sha1);
	upload(c[i]);
	//Get("/home/creuvard/file.crv");
	*/
	server = give_server();
	if (server == NULL) {
		fprintf(stderr, "%s\n", "You must choose server id");
		return;
	}

	cat = choice_cat();
	if (cat == NULL) {
		fprintf(stderr, "%s", "Creuvux fail to fetch categories list");
		return (-1);
	}
	fprintf(stdout, "Catégorie: %s\n", cat);
	for (i = 0; c[i]; i++) {
			printf("-> %s\n", c[i]);
			upload(c[i], cat, server);
			printf("DEBUG avant fin loop\n");
	}
	printf("DEBUG (avant fin main function\n");
	
	
	// Send
	//upload(c[i]);	

	Free_options();
	return (0);
}
