/**
 * \file creuvuxd.c
 * \brief Fichier principale.
 * \author Sylvain Bourdou
 * \version 0.81
 * \date 25 Février 2013
 *
 * Création d'un serveur
 *
 */


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
#include <fcntl.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>


/*
 * SQLite3 Stuff
 */
#include <sqlite3.h>


/*
 * Local Stuff
 */
#include "creuvard.h"
#include "server_conf.h"
#include "help.h"
#include "network.h"
#include "create_database.h"

/* Server configuration options. */
ServerOptions options;


/*
 * usage()
 */
/**
 * \fn void usage (const char *str)
 * \brief Affiche l'aide.
 *
 * \param str Chaine de caractère.
 */

static  void
usage (const char *str)
{
  if (str)
    printf ("\n%s\n", str);

  fprintf (stdout, "%s\n", "Usage : creuvuxd [-hvca] [-f config_file]" );
	fprintf (stdout, "%s\n", "\t-h\t\t\t Show this help");
	fprintf (stdout, "%s\n", "\t-v\t\t\t Show version");
	fprintf (stdout, "%s\n", "\t-u\t\t\t Update database" );
	fprintf (stdout, "%s\n", "\t-a\t\t\t Add users present into group.conf in DB" );
	fprintf (stdout, "%s\n", "\t-g /directory/\t\t Add group access to directory" );
	fprintf (stdout, "%s\n", "\t-g /directory/file\t Add group access to file" );
	fprintf (stdout, "%s\n", "\t-f file\t\t\t Take an alternative configuration file, default /etc/creuvuxd/creuvuxd.conf" );
}

/*
 * Get passphrase
 */
/**
 * \fn int get_passwd (void)
 * \brief Récupère le mot de passe.
 */
static int
get_passwd (void)
{
  char *cr;
  int result = -1;
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


/*
 * read_server_conf(const char *filename)
 * -> Read configuration file and set options.
 */

/**
 * \fn static int read_server_conf(const char *filename)
 * \brief Lis le fichier de configuration et charge l'environnement.
 *
 * \param filename Path et fichier de configuration (Par défaut /etc/creuvuxd/creuvux.conf).
 * \return -1 en cas d'erreur, et 0 si tout va bien.
 */
static int 
read_server_conf(const char *filename)
{
  FILE *fd = NULL;
  char buf[SIZE];
  int nb_sep = 0;
  int i;
  char **param = NULL;
  
  fd = crv_fopen(filename, "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "read_server_conf(): Err[001] crv_fopen() failed");
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

			if (!crv_strncmp( "CREUVUX_USER", param[0]))
				options.user = crv_strdup(param[1]);
	  
			if (!crv_strncmp( "CREUVUX_PATH", param[0]))
				options.path = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_DEBIT", param[0]))
			{
				long lval = 0;
				char *ep;
				errno = 0;
				lval = strtol(param[1], &ep, 10);
				if (buf[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is not a number");
					break;
				}
				if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || 
						(lval > INT_MAX || lval < INT_MIN)) 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is out of range");
					break;
				}
				options.bandwidth = lval;
			}

			if (!crv_strncmp( "CREUVUX_MAX_SIZE", param[0]))
			{
				long lval = 0;
				char *ep;
				errno = 0;
				lval = strtol(param[1], &ep, 10);
				if (buf[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is not a number");
					break;
				}
				if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || 
						(lval > INT_MAX || lval < INT_MIN)) 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is out of range");
					break;
				}
				options.db_max = lval;
			}

			if (!crv_strncmp( "CREUVUX_MIN_SIZE", param[0]))
			{
				long lval = 0;
				char *ep;
				errno = 0;
				lval = strtol(param[1], &ep, 10);
				if (buf[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is not a number");
					break;
				}
				if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || 
						(lval > INT_MAX || lval < INT_MIN)) 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is out of range");
					break;
				}
				options.db_min = lval;
			}



			if (!crv_strncmp( "CREUVUX_PORT", param[0]))
			{
				long lval = 0;
				char *ep;
				errno = 0;
				lval = strtol(param[1], &ep, 10);
				if (buf[0] == '\0' || *ep != '\0') 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is not a number");
					break;
				}
				if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
						(lval > INT_MAX || lval < INT_MIN)) 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is out of range");
					break;
				}
				options.num_ports = lval;
			}
	  
			if (!crv_strncmp( "CREUVUX_ADDR", param[0]))
				options.listen_addrs = crv_strdup(param[1]);
	  
			if (!crv_strncmp( "CREUVUX_HOST", param[0]))
				options.host = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_PID", param[0]))
				options.pid = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_SEC", param[0]))
			{
				if (!crv_strncmp( "no", param[1]))
					options.sec = -1;
				else if (!crv_strncmp( "yes", param[1]))
					options.sec = 1;
				else 
				{
					fprintf(stderr, "%s\n", "Please check CREUVUX_SEC=\"yes|no\"");
					break;
				}
			}

			if (!crv_strncmp( "CREUVUX_DEBUG", param[0]))
			{
				if (!crv_strncmp( "no", param[1]))
					options.debug = -1;
				else if (!crv_strncmp( "yes", param[1]))
					options.debug = 1;
				else 
				{
					fprintf(stderr, "%s\n", "Please check CREUVUX_DEBUG=\"yes|no\"");
					break;
				}
			}

			if (!crv_strncmp( "CREUVUX_PWD", param[0]))
				options.passphrase = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_IPv", param[0]))
			{
				if (!crv_strncmp( "4", param[1]))
					options.address_family = AF_INET;
				else if (!crv_strncmp( "6", param[1]))
					options.address_family = AF_INET6;
				else 
				{
					fprintf(stderr, "%s\n", "Please check CREUVUX_IPv=\"4|6\"");
					break;
				}
			}
	  
			if (!crv_strncmp( "CREUVUX_CHROOT", param[0]))
				options.chroot_directory = crv_strdup(param[1]);
	  
			if (!crv_strncmp( "CREUVUX_UPDIR", param[0]))
				options.upload_directory = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_PUBLIC", param[0]))
				options.public_directory = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_IPDB", param[0]))
				options.ipdb = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_PORTDB", param[0]))
			{
				long lval = 0;
				char *ep;
				errno = 0;
				lval = strtol(param[1], &ep, 10);
				if (buf[0] == '\0' || *ep != '\0') 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is not a number");
					break;
				}
				if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
						(lval > INT_MAX || lval < INT_MIN)) 
				{
					fprintf(stderr, "%s%s%s\n", "\"", param[1], "\" is out of range");
					break;
				}
				options.portdb = lval;
			}
			
			if (!crv_strncmp( "CREUVUX_DBNAME", param[0]))
				options.dbname = crv_strdup(param[1]);

			if (!crv_strncmp( "CREUVUX_DBUSER", param[0]))
				options.dbuser = crv_strdup(param[1]);
			
			if (!crv_strncmp( "CREUVUX_DBPASSW", param[0]))
				options.dbpassw = crv_strdup(param[1]);

			nb_sep = 0;
			for (i = 0; param[i]; i++)
				crv_free(param[i]);
			crv_free(param);
		}
  }
  
	if (!feof(fd)) 
	{
		fclose(fd);
		return (-1);
  }
  fclose(fd);

  return (0);
}

/*
 * Check if files creuvux.log, creuvux.stat and listing.xml exist 
 */

/**
 * \fn static int check_env(char *chroot_dir)
 * \brief Verifie le path de chroot et cree le fichier de log.
 *
 * \param chroot_dir Path dans lequel le serveur va se chroote
 * \return -1 en cas d'erreur, et 0 si tout va bien.
 */
static int check_env(char *chroot_dir)
{
  int result = -1;
  FILE *fd = NULL;

  result = crv_chdir (options.chroot_directory);
  if (result == -1) {
		fprintf( stderr, "%s\n", "check_env(): Err[001] crv_chdir() failed");
		return (-1);
  }

  fd = crv_fopen("creuvux.log", "w");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "check_env(): Err[003] crv_fopen(\"creuvux.log\", \"w\"); failed");
		return (-1);
  }
  fclose(fd);
	fd = NULL;

  return (0);
}


/*
 * Main program for daemon.
 */
/**
 * \fn int main (int argc, char **argv)
 * \brief Fonction principale.
 *
 * \param argv Argument passés en ligne de commande
 * \return -1 en cas d'erreur, et 0 si tout va bien.
 */
int main (int argc, char **argv)
{
  /* extern char *optarg; */
  /* extern int optind; */

  int opt;

	/* update flag: 
	 * '0' -> Don't Create/Update database, 
	 * '1' -> Create/Update database
	 */
	int update = 0;
	
	int result = -1;
	
	/* check options.chroot_directory */
	struct stat st; 
  
	/* Check *.pem files */
	FILE *fd;		   
  char path[SIZE];

  /* Ensure that standart files descriptor 0, 1 and 2 are open or directed to /dev/null */
  crv_sanitise_stdfd();
  
  /* Initialize configuration options to their default values. */
  initialize_server_options(&options);

  while ((opt = getopt(argc, argv, "f:hvsuga")) != -1)
  {
		switch(opt) {
			case 'h': /* Print Help */
				fprintf(stdout, "%s\n", "Help:");
				usage (NULL);
				exit(1);
			case 'v': /* Print version */
				fprintf (stderr, "%s%s\n", "creuvuxd version ", CREUVUX_VERSION);
				fprintf (stderr, "%s\n", 
					"Before reporting bugs (creuvard@colerix.org) please check "
					"the development CHANGELOG to make sure it hasn't already "
					"been fixed in devel.");
				exit(1);;
			case 'f': /* Config File */
				crv_free(options.config);
				options.config = crv_strdup(optarg);
				break;
			case 'u': /* create Database */
				update = 1;
				break;
			case '?':
			default:
				usage(NULL);
				exit(1);
		}
  }
  argc -= optind;
  argv += optind;

  /* Read configuration file and set options */
  result = read_server_conf (options.config);
  if (result == -1) {
    Free_options();
		return (-1);
	}

	/* Check if passphrase isn't NULL */
	if (options.passphrase == NULL) {
		fprintf(stdout, "%s", "Enter your passphrase:");
		fflush(stdin);
		get_passwd ();
		fprintf(stdout, "%s", "\n");
	}
 
	/* Change user id for options checking */
	result = crv_drop_priv_temp (options.user);
	if (result == -1) {
		fprintf(stderr, "%s", "Change User ID failed\n");
		Free_options();
		return (-1);
	}

  /* Check if files creuvux.log, creuvux.stat and listing.xml exist */
  result = check_env(options.chroot_directory);
  if (result == -1) {
		fprintf( stderr, "%s%s%s\n", "main(): check_env(", options.chroot_directory, ") failed");
		Free_options();
		return (-1);
  }
  
  if (optind < argc) {
		fprintf( stderr, "%s%s\n", "Extra argument ", argv[optind]);
		Free_options();
		exit(EXIT_FAILURE);
  }

  /*
   * Check Configuration 
   */

  /* Chech User */
  if (getpwnam(options.user) == NULL) {
		fprintf(stderr, "Privilege separation user %s does not exist", options.user);
		Free_options();
		exit(1);
  }

  /* Check chroot_directory */
  if ((stat(options.chroot_directory, &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
		fprintf( stderr, "Missing privilege separation directoty: %s\n", options.chroot_directory);
		Free_options();
		exit(1);
  }

  /* Check upload_directory */
  (void)crv_strncpy(path, options.chroot_directory, sizeof(path));
  (void)crv_strncat(path, options.upload_directory, sizeof(path));
  if ((stat(path, &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
		fprintf( stderr, "Missing upload directory (in chroot jail ->'%s'): %s\n", options.chroot_directory, options.upload_directory);
		Free_options();
		exit(1);
  }
  memset(path, 0, sizeof(path));

  /* Check public_directory */
  (void)crv_strncpy(path, options.chroot_directory, sizeof(path));
  (void)crv_strncat(path, options.public_directory, sizeof(path));
  if ((stat(path, &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
		fprintf( stderr, "Missing public directory (in chroot jail): %s\n", options.public_directory);
		Free_options();
		exit(1);
  }
  memset(path, 0, sizeof(path));


  /* Check config directory */
  if ((stat(options.path, &st) == -1) || (S_ISDIR(st.st_mode) == 0)) {
		fprintf( stderr, "Option CREUVUX_PATH is missing: %s\n", options.path);
		Free_options();
		exit(1);
  }

	/* Change directory to options.path (default: /etc/creuvuxd/ ) */
  result = crv_chdir(options.path);
  if (result == -1) {
		fprintf( stderr, "%s%s%s\n", "main(): crv_chdir(", options.path,") failed");
		Free_options();
		exit(1);
  }
  

  /* Check server.pem */
  fd = crv_fopen("server.pem", "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Missing server.pem.");
		Free_options();
		return (-1);
  }
  fclose(fd);
  
  /* Check rootcert.pem */
  fd = crv_fopen("rootcert.pem", "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Missing rootcert.pem");
		Free_options();
		return (-1);
  }
  fclose(fd);
  
  /* Check group.conf */
  fd = crv_fopen("group.conf", "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Missing group");
		Free_options();
		return (-1);
  }
  fclose(fd);

	/* Create database */
  if (update == 1)
  {
		sqlite3 *db = NULL;
		char *zErrMsg = 0;
		int rc = -1;

		/* Restore privilege */
		result = crv_restore_priv();
		if (result == -1) {
			fprintf(stderr, "%s", "Creuvuxd can't restore privilege\n");
			Free_options();
			return (-1);
		}

		/* Drop privilege */
		/* Chroot process */
		/* A décommenter */
		result = crv_drop_priv_perm_and_chroot (options.user , options.chroot_directory );
		if (result == -1) {
			fprintf( stderr, "%s%s%s%s%s", "\nmain(): drop_priv_perm_and_chroot(", 
																	options.user, 
																 ",",
																 options.chroot_directory,
																 ") failed\n");
			fprintf( stderr, "%s", "main(): Please check you are ROOT\n");
			Free_options();
			return (-1);
		}
		
		
		
		fprintf(stdout, "%s", "Sqlite DB creation ");
		fflush(stdout);
		rc = sqlite3_open( ".creuvux.db", &db);
		if( rc ){
			fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
			sqlite3_close(db);
			Free_options();
			return (-1);
		}
		fprintf(stdout, "%s\n", " [Ok] ");
		
		
		fprintf(stdout, "%s\n", "Listing direcory and update database...");
		/*
		 * Multiple TABLE CREATION (Fichiers, Categories, Descr, Grp_User, Grp_Sha1, Descr)
		 */
	
		/*
		 * TABLE Files
		 */
		
		
		rc = sqlite3_exec(db, "create table Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL, Path TEXT NOT NULL);" , NULL, 0, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL (Table=Files)  error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		
		
		list_directory_and_create_db(options.chroot_directory, db);
		//list_directory_and_create_db( "/", db);
		sqlite3_close(db);
		Free_options();
		return (0);
  }
  
  /* ignore SIGPIPE */
  signal (SIGPIPE, SIG_IGN);

  signal(SIGCHLD, crv_sigchld_handler);
	
	/* Restore privilege */
	result = crv_restore_priv();
	if (result == -1) {
		fprintf(stderr, "%s", "Creuvuxd can't restore privilege\n");
		Free_options();
		return (-1);
	}

  if (options.debug == 1)
  {
		fprintf(stderr, "%s\n", "#################");
		fprintf(stderr, "%s\n", "# MODE DEBUG ON #");
		fprintf(stderr, "%s\n", "#################");
		fprintf(stderr, "# \t| \n");
		fprintf(stderr, "# \t|-> CREUVUX_USER    :'%s'\n", options.user);
		fprintf(stderr, "# \t|-> CREUVUX_PATH    :'%s'\n", options.path);
		fprintf(stderr, "# \t|-> CREUVUX_DEBIT   :'%d'\n", options.bandwidth);
		fprintf(stderr, "# \t|-> CREUVUX_PORT    :'%d'\n", options.num_ports);
		fprintf(stderr, "# \t|-> CREUVUX_ADDR    :'%s'\n", options.listen_addrs);
		fprintf(stderr, "# \t|-> CREUVUX_HOST    :'%s'\n", options.host);
		fprintf(stderr, "# \t|-> CREUVUX_PID     :'%s'\n", options.pid);
		fprintf(stderr, "# \t|-> CREUVUX_SEC     :'%d'\n", options.sec);
		fprintf(stderr, "# \t|-> CREUVUX_PWD     :'%s'\n", options.passphrase);
		fprintf(stderr, "# \t|-> CREUVUX_CHROOT  :'%s'\n", options.chroot_directory);
		fprintf(stderr, "# \t|-> CREUVUX_UPDIR   :'%s'\n", options.upload_directory);
		fprintf(stderr, "# \t|-> CREUVUX_PUBLIC  :'%s'\n", options.public_directory);
		fprintf(stderr, "# \t|-> CREUVUX_IPDB    :'%s'\n", options.ipdb);
		fprintf(stderr, "# \t|-> CREUVUX_PORTDB  :'%d'\n", options.portdb);
		fprintf(stderr, "# \t|-> CREUVUX_DBNAME  :'%s'\n", options.dbname);
		fprintf(stderr, "# \t|-> CREUVUX_DBUSER  :'%s'\n", options.dbuser);
		fprintf(stderr, "# \t|-> CREUVUX_DBPASSW :'%s'\n", options.dbpassw);
		fprintf(stderr, "# \t|-> CREUVUX_MAX_SIZE:'%lld'\n", options.db_max);
		fprintf(stderr, "# \t`-> CREUVUX_MIN_SIZE:'%lld'\n", options.db_min);
		fprintf(stderr, "%s\n\n", "#################");
  }
  else if (options.debug == -1) {
		int fd_pid;
		char str[12];
		/* Put pid_file */
		if (options.pid == NULL) {
			fprintf(stderr, "%s", "creuvuxd failed, CREUVUXD_PID is NULL\n");
			Free_options();
			exit(EXIT_FAILURE);
		}
		fd_pid = open( options.pid, O_RDWR | O_CREAT, 0640);
		if (fd_pid < 0) {
			fprintf(stderr, "creuvuxd can't open PID file\n");
			Free_options();
			exit(EXIT_FAILURE);
		}

		if (lockf (fd_pid, F_TLOCK, (off_t) 0) < 0) {
			fprintf(stderr, "%s", "creuvuxd is already running\n");
			Free_options();
			exit(EXIT_FAILURE);
		}

		fprintf(stdout, "%s\n", "Mode Daemon");
		crv_daemon_mode();
		snprintf (str, 12, "%d\n", getpid());
		(void)write (fd_pid, str, strlen(str));
  }
 
  server_accept_loop ( );
  Free_options();
	fprintf(stderr, "%s", "Creuvuxd Received terminating signal\n");
  return 0;
}
