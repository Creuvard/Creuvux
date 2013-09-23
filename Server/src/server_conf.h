/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef SERVER_CONF
#define SERVER_CONF

typedef struct {
  int				  num_ports;			/* Assign a port to listen. */
  char				  *user;				/* Name user which users are mapped to. */
  char				  *path;				/* Configurations files path (groups.xml, creuvuxd.conf, rootcert.pem, server.pem ).*/
  int				  bandwidth;			/* Maximum data transfer rate permitted, in KBps. */
  char				  *chroot_directory;	/* Chroot directory */
  char				  *config;				/* Configuration file*/
  /*struct sockaddr_in  *listen_addrs;		*//* Assign an address to listen. */
  char				  *listen_addrs;		/* Assign an address to listen. */
  int				  address_family;		/* Address family used by the server. */
  char				  *host;				/* Assign FQDN for listing.xml */
  int				  debug;				/* Run with on debug mode. */
  char				  *pid;					/* Name of pid file. */
  int				  sec;					/* Security level. */
  char				  *passphrase;			/* Passphrase. */
  char				  *upload_directory;	/* Upload directory. */
  char				  *public_directory;	/* Directory where files are read by anonymous connexion. */
  char				  *stat_repport;		/* Filename of HTML repport */
  off_t				  db_max; 	/* Max size of file for db entry */
  off_t 			  db_min; 	/* Min size of file for db entry */  
	char 					*ipdb;    /* Ip adress of DataBase */
	int 					*portdb;  /* Port use by MySQL DB */
	char 					*dbname;  /* Name of Database */
	char 					*dbuser;  /* User who allow to connect to Database */
	char 					*dbpassw; /* Password use by user allow to connect to database*/
} ServerOptions;

extern void initialize_server_options ( ServerOptions *);
extern void Free_options(void);

#endif /* SERVER_CONF */
