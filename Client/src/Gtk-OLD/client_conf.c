/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "client_conf.h"
#include "creuvard.h"


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
}
