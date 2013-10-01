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
#include <libgen.h>

#include <sys/types.h>
#include <sys/stat.h>

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

#include "sqlite3.h"



#include "creuvard.h"
#include "client_conf.h"
#include "Gtk_gui.h"
/*
#include "help.h"
#include "sync.h"
#include "get.h"
#include "put.h"
#include "msg.h"
#include "find.h"
#include "list.h"
#include "info.h"
#include "list_grp.h"
#include "list_cat.h"
*/

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
 * Main program for daemon.
 */
int main (int argc, char **argv)
{
  int opt;
  int result = -1;
  struct stat st; /* check options.chroot_directory */
  FILE *fd;		   /* Check *.pem files */
  char *command = NULL;
  char buf[SIZE];
  char **c = NULL;
  int i = 0;
  char **arg = NULL; /* Use when creuvux is run with option from command line */
  int arg_inc = 0;
	int mode_script = -1; /* if mode_script = 1, script mode is enable */

    sqlite3 *db = NULL;
	char *zErrMsg = 0;
	int rc = -1;

	
  /* Ensure that standart files descriptor 0, 1 and 2 are open or directed to /dev/null */
  //crv_sanitise_stdfd();

  /* Initialize configuration options to their default values. */
  initialize_client_options(&options);
	options.gui = 1;

  /* Read configuration file and set options */
  result = read_client_conf (options.config);
  if (result == -1)
    return (-1);

	/* Check if passphrase isn't NULL */
	if (options.passphrase == NULL) {
		fprintf(stdout, "%s", "Edit creuvux.conf and add CREUVUX_PWD=\"password\"\n");
		return (-1);
	}

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

  result = chdir(options.path);
  if (result == -1) {
	fprintf( stderr, "%s%s%s\n", "main(): chdir(", options.path,") failed");
	exit(1);
  }

  /* Check existence of listing directory */
  if ((stat( "./listing", &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
	fprintf( stderr, "Directory %slisting/ is missing\n", options.listing_directory);
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
	/*
	if ( options.address_family == AF_INET)
		fprintf(stderr, "%s", "# |-> CREUVUX_ADDR  :'IPv4'\n");
	else if ( options.address_family == AF_INET6)
		fprintf(stderr, "%s", "# |-> CREUVUX_ADDR  :'IPv6'\n");
	*/
	if (options.compression == 1)
	fprintf(stderr, "%s", "# |-> CREUVUX_COMPRESSION  :'Yes'\n");
	else if (options.compression == -1)
	fprintf(stderr, "%s", "# |-> CREUVUX_COMPRESSION  :'No'\n");
	fprintf(stderr, "%s\n\n", "#################");
  }


	/* Open MAIN Database */

	rc = sqlite3_open("Database.db", &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		for (i = 0; c[i]; i++)
			crv_free(c[i]);
		crv_free(c);
		Free_options();
		exit(1);
	}

	gen_Gtk_gui(argc, argv, db);
	sqlite3_close(db);
	Free_options();
	return (0);
}
