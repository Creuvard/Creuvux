/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef CLIENT_CONF
#define CLIENT_CONF

typedef struct {
  char	  *path;								/* Configurations files path (groups.xml, creuvux.conf, rootcert.pem, server.pem ).*/
  char	  *config;							/* Configuration file*/
  int			debug;								/* Run with on debug mode. */
  int			sec;									/* Security level. */
  char	  *passphrase;					/* Passphrase. */
  char	  *download_directory;	/* Upload directory. */
  char	  *listing_directory;		/* Listing directory. */
  char	  *tmp_directory;				/* temp directory. */
  int			address_family;				/* Ipv4 or IPv6 */
	int			compression;					/* Data compression */

  int			gui;									/* 1=Gui active lese  Gui = 0 */
  char	  *gui_addr;						/* list of adress */
  int			gui_port;							/* Listen port */
  char	  *srv;									/* Favorite server */
  char	  *term_exec;						/* path for xterm binarie */
  char	  *crv_exec;						/* path for creuvux binarie */
} ClientOptions;

extern void initialize_client_options ( ClientOptions *);

#endif /* CLIENT_CONF */
