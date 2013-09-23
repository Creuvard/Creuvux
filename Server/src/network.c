/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>


/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

/* ZLIB Stuff */
#include <zlib.h>


/*
 * SQLite3 Stuff
 */
#include <sqlite3.h>


#include "creuvard.h"
#include "network.h"
#include "log.h"
#include "server_conf.h"
#include "SSL_sendfile.h"
#include "SSL_loadfile.h"
#include "put.h"
#include "sync.h"
#include "get.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);


static volatile sig_atomic_t received_sigterm = 0;

/* Server configuration options. */
extern ServerOptions options;

/* Command options. */
Command_options command;

#define MAX_LISTEN_SOCKETS	16
int listen_socks[MAX_LISTEN_SOCKETS];
int num_listen_socks = 0;

struct timeval tv1, tv2;


static void
handle_error (const char *file, int lineno, const char *msg)
{
  fprintf (stderr, "%s %d %s\n", file, lineno, msg);
  ERR_print_errors_fp (stderr);
}


/*
 * Initialisation OpenSSL
 */
static void
init_OpenSSL (void)
{
  if (!SSL_library_init ())
    {
      fprintf (stderr, "%s\n", "** OpenSSL initialization failed!");
      exit (EXIT_FAILURE);
    }
  SSL_load_error_strings ();
}

static int
verify_callback (int ok, X509_STORE_CTX * store)
{
  char data[256];

  if (!ok)
    {
      X509 *cert = X509_STORE_CTX_get_current_cert (store);
      int depth = X509_STORE_CTX_get_error_depth (store);
      int err = X509_STORE_CTX_get_error (store);

      fprintf (stderr, "%s%i\n", "-Error with certificate at depth: ", depth);
      X509_NAME_oneline (X509_get_issuer_name (cert), data, 256);
      fprintf (stderr, "%s%s\n", "  issuer   = ", data);
      X509_NAME_oneline (X509_get_subject_name (cert), data, 256);
      fprintf (stderr, "%s%s\n", "  subject  = ", data);
      fprintf (stderr, "%s\n", X509_verify_cert_error_string (err));
      fprintf (stderr, "  err %i:%s\n", err,
	       X509_verify_cert_error_string (err));
    }

  return ok;
}


static long
post_connection_check (SSL * ssl, char *host)
{
  X509 *cert;
  X509_NAME *subj;
  char data[256];
  int extcount;
  int ok = 0;

  if (!(cert = SSL_get_peer_certificate (ssl)) || !host)
    goto err_occured;
  if ((extcount = X509_get_ext_count (cert)) > 0)
    {
      int i;

      for (i = 0; i < extcount; i++)
	{
	  char *extstr;
	  X509_EXTENSION *ext;

	  ext = X509_get_ext (cert, i);
	  extstr =
	    (char *)
	    OBJ_nid2sn (OBJ_obj2nid (X509_EXTENSION_get_object (ext)));

	  if (!strcmp (extstr, "subjectAltName"))
	    {
	      int j;
	      unsigned char *data_;
	      STACK_OF (CONF_VALUE) * val;
	      CONF_VALUE *nval;
	      X509V3_EXT_METHOD *meth;
	      void *ext_str = NULL;

	      if (!(meth = X509V3_EXT_get (ext)))
		break;
	      data_ = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
	      if (meth->it)
		ext_str = ASN1_item_d2i (NULL, &data_, ext->value->length,
					 ASN1_ITEM_ptr (meth->it));
	      else
		ext_str = meth->d2i (NULL, &data_, ext->value->length);
#else
	      ext_str = meth->d2i (NULL, &data_, ext->value->length);
#endif
	      val = meth->i2v (meth, ext_str, NULL);
	      for (j = 0; j < sk_CONF_VALUE_num (val); j++)
		{
		  nval = sk_CONF_VALUE_value (val, j);
		  if ((!strcmp (nval->name, "DNS")
		       && !strcmp (nval->value, host)) | (!strcmp (nval->name,
								   "IP Address")
							  && !strcmp (nval->
								      value,
								      host)))
		    {
		      ok = 1;
		      break;
		    }
		}
	    }
	  if (ok)
	    break;
	}
    }

  if (!ok && (subj = X509_get_subject_name (cert)) &&
      X509_NAME_get_text_by_NID (subj, NID_commonName, data, 256) > 0)
    {
      data[255] = 0;
      if (strcasecmp (data, host) != 0)
	goto err_occured;
    }

  X509_free (cert);
  return SSL_get_verify_result (ssl);

err_occured:
  if (cert)
    X509_free (cert);
  return X509_V_ERR_APPLICATION_VERIFICATION;
}



static void
seed_prng (void)
{
  RAND_load_file ("/dev/urandom", 1024);
}


char *
recup_pseudo (SSL * ssl)
{
  X509 *cert;
  char value[SIZE];
  char *pseudo;
  if (!(cert = SSL_get_peer_certificate (ssl)))
    {
      return NULL;
    }

  X509_NAME_get_text_by_NID (X509_get_subject_name (cert),
			     NID_commonName, value, SIZE);
  pseudo = crv_strdup ((const char *) value);
  X509_free (cert);
  return (pseudo);
}


char *
recup_email (SSL * ssl)
{
  X509 *cert;
  char value[SIZE];
  char *email;

  if (!(cert = SSL_get_peer_certificate (ssl)))
    {
      return NULL;
    }
  X509_NAME_get_text_by_NID (X509_get_subject_name (cert),
			     NID_pkcs9_emailAddress, value, SIZE);
  email = crv_strdup ((const char *) value);
  X509_free (cert);
  return (email);
}

DH *dh512 = NULL;
DH *dh1024 = NULL;


static void
init_dhparams (void)
{
  BIO *bio;

  bio = BIO_new_file ("dh512.pem", "r");
  if (!bio)
    {
      int_error ("dh512.pem INTROUVABLE");
      exit (EXIT_FAILURE);
    }
  dh512 = PEM_read_bio_DHparams (bio, NULL, NULL, NULL);
  if (!dh512)
    {
      int_error ("ERREUR DE LECTURE de dh512.pem");
      exit (EXIT_FAILURE);
    }
  BIO_free (bio);

  bio = BIO_new_file ("dh1024.pem", "r");
  if (!bio)
    {
      int_error ("dh1024.pem INTROUVABLE");
      exit (EXIT_FAILURE);
    }
  dh1024 = PEM_read_bio_DHparams (bio, NULL, NULL, NULL);
  if (!dh1024)
    {
      int_error ("ERREUR DE LECTURE de dh1024.pem");
      exit (EXIT_FAILURE);
    }
  BIO_free (bio);
}

static DH *
tmp_dh_callback (SSL * ssl, int is_export, int keylength)
{
  DH *ret;

  if (!dh512 || !dh1024)
    init_dhparams ();

  switch (keylength)
    {
    case 512:
      ret = dh512;
      break;
    case 1024:
    default:
      ret = dh1024;
      break;
    }
  return ret;
}

static int
passwd_cb (char *buf, int size, int rwflag, void *passwd)
{
  strncpy (buf, (char *) (options.passphrase), (size_t) size);
  buf[size - 1] = '\0';
  return (strlen (options.passphrase));
}

#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"
#define CAFILE "rootcert.pem"
#define CADIR NULL
#define CERTFILE "server.pem"
static SSL_CTX *
setup_server_ctx (void)
{
  SSL_CTX *ctx;

  ctx = SSL_CTX_new (SSLv23_method ());
  /* Test de la passphrase */
  SSL_CTX_set_default_passwd_cb (ctx, passwd_cb);

  if (SSL_CTX_load_verify_locations (ctx, CAFILE, CADIR) != 1)
    int_error ("ERREUR DE CHARGEMENT DU FICHIER");
  if (SSL_CTX_set_default_verify_paths (ctx) != 1)
    int_error ("ERREUR DE CHARGEMENT DU FICHIER default CA file");
  if (SSL_CTX_use_certificate_chain_file (ctx, CERTFILE) != 1)
    int_error ("ERREUR DE CHARGEMENT DU CERTIFICAT");
  if (SSL_CTX_use_PrivateKey_file (ctx, CERTFILE, SSL_FILETYPE_PEM) != 1)
    int_error ("ERREUR DE CHARGEMENT DE LA CLEF PRIVE");
  SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
		      verify_callback);
  SSL_CTX_set_verify_depth (ctx, 4);
  SSL_CTX_set_options (ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
		       SSL_OP_SINGLE_DH_USE);
  
  SSL_CTX_set_tmp_dh_callback (ctx, tmp_dh_callback);
  
  if (SSL_CTX_set_cipher_list (ctx, CIPHER_LIST) != 1)
    int_error ("ERREUR DE LECTURE DE LA LISTE DE CHIPTER");
  return ctx;
}


void
initialize_command_opt(Command_options *command)
{
  memset(command, 0, sizeof(*command));
  command->name = NULL;
  command->version = NULL;
  command->user = NULL;
  command->mail = NULL;
  command->lst_file = NULL;
  command->addr = NULL;
  command->sha1 = NULL;
  command->begin = 0;
  command->end = 0;
  command->grp = NULL;
  command->path = NULL;
  command->flag = 0;
  command->filename = NULL;
  command->old_sha1 = NULL;
	command->genre = NULL;
	command->comment = NULL;
}


static int
pre_authcon(SSL *ssl, const char *xpath)
{
  char buf[4096];
  char *cmd = NULL;
  ssize_t nb = 0;
  char **c;
  int i;

  
  fprintf( stderr, 
	  "DEBUG:\n\t"
	  "user     : '%s'\n\t"
	  "mail     : '%s'\n\t"
	  "lst_file : '%s'\n\t"
	  "addr     : '%s'\n", 
	  command.user, command.mail, command.lst_file, command.addr);
	 

  /* DATA EXCHANGE - Receive message and send reply. */
  nb = SSL_read (ssl, buf, sizeof(buf) - 1);
  if ( nb < 0) {
		Log ( "CRASH", 0, "%s\n", "pre_authcon(): Err[002] SSL_read() failed" );
		return (-1);
  }

  buf[nb] ='\0';
  if ((cmd = strdup(buf)) == NULL)
    return (-1);
  
  memset(buf, 0, sizeof(buf));
  
	if (options.debug == 1)
		fprintf(stderr, "DEBUG: Commande recue -->'%s'\n", cmd);
  
	c =  crv_cut(cmd, "#");
  
	command.version = crv_strdup(c[1]);

  
  /*********/
  /*  GET  */
  /*********/
  if (!crv_strncmp(c[0], "GET")) {
		if (options.debug == 1)
			fprintf(stderr, "DEBUG: Pre_Get() is running\n");
		if (pre_Get(c) == -1)
			Log ( "CRASH", 0, "%s\n", "Pre_Get() Failed");
  } /* End of GET */
  /*******/
  /* PUT */
  /*******/
  else
  if (!crv_strncmp(c[0], "PUT")) 
  {
		char *ep = NULL;
		long lval = 0;
		errno = 0;
		command.name = crv_strdup("PUT");
		command.sha1 = crv_strdup(c[2]);
		
		lval = strtol(c[3], &ep, 10);
		
		if (c[3][0] == '\0' || *ep != '\0') {
		  Log ( "CRASH", 0, "%s\n", "Command format is wrong, begin is not a number");
		  return (-1);
		}
		
		if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
		  (lval > INT_MAX || lval < INT_MIN))
		{
		  Log ( "CRASH", 0, "%s\n", "Command format is wrong, begin is out of the rang");
		  return (-1);
		}
		
		command.begin = lval;
		/* Transforme c[3] and c[4] in off_t */
		lval = strtol(c[4], &ep, 10);
		if (c[4][0] == '\0' || *ep != '\0') {
			//Log ( "CRASH", ssl, "%s\n", "Command format is wrong, end is not a number");
			return (-1);
		}
		if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
			(lval > INT_MAX || lval < INT_MIN))
		{
			Log ( "CRASH", 0, "%s\n", "Command format is wrong, end is out of the rang");
			return (-1);
		}
		command.end = (off_t) lval;
		/*
		 * Faut vérifier que le client à le droit de deposer le fichier et 
		 * ensuite SEULEMENT le lui permettre.
		 */
		/* On verra plus tard, en essayant de rajouter un listing des groupes autorisée
		 * a uploader.
		result = get_grp_user(command.user);
		if (result == -1) {
			Log ( "CRASH", ssl, "%s\n", "Can't get group of user");
			return (-1);
		}
		*/
	
  }  /* End of PUT */

   /*fprintf(stderr, "DEBUG: '%s'\n", cmd);*/
  
  for (i = 0; c[i]; i++)
		crv_free(c[i]);
  crv_free(c);
  
  return (0);
}


static int
traitement_conec (SSL * ssl)
{
  char *user = NULL;
  int result = -1;
  user = recup_pseudo (ssl);


  /*********/
  /*  GET  */
  /*********/
  if (!crv_strncmp(command.name, "GET"))
  {
		result = Get(ssl, command.filename, command.begin, command.end, command.path);	
		if (result == -1)
			return (-1);
  } else
	/*****************/
  /*  PUT/COMMENT  */
  /*****************/
  if (!crv_strncmp(command.name, "PUT") ||
      !crv_strncmp(command.name, "COMMENT"))
  {
		if (!crv_strncmp(command.name, "PUT"))
			Log ("OK", 1, "PUT");
		
		result = SSL_loadfile (ssl, command.sha1); 
		if (result == -1) {
			Log ("ERROR", 1, "PUT/COMMENT -> FAILED");
			return (-1);
		}

		if (!crv_strncmp(command.name, "PUT"))
			Log ("OK", 1, "PUT -> OK");
  }
 
  return (0);
}


static void
Free_cmd_struct(void)
{
  if ( command.name!= NULL)
	crv_free(command.name);
  if ( command.version!= NULL)
	crv_free(command.version);
  if (command.user != NULL)
	crv_free(command.user);
  if (command.mail != NULL)
	crv_free(command.mail);
  if (command.lst_file != NULL)
	crv_free(command.lst_file);
  if ( command.addr!= NULL)
	crv_free(command.addr);
  if ( command.sha1!= NULL)
	crv_free(command.sha1);
	if (command.grp != NULL)
	crv_free(command.grp);
  if ( command.path!= NULL)
	crv_free(command.path);
  if ( command.filename!= NULL)
	crv_free(command.filename);
  
}

/*
 * Close all listening sockets
 */
static void
close_listen_socks(void)
{
  int i;
  for (i = 0; i < num_listen_socks; i++)
	close(listen_socks[i]);
  num_listen_socks = -1;
}

/*
 * Generic signal handler for terminating signals in the master daemon.
 */
/*ARGSUSED*/
static void
sigterm_handler(int sig)
{
	received_sigterm = sig;
}

/* Interception de Ctrl^C */
static
void ctr_c(int signum)
{
	fprintf(stderr, "%s", "\n- Creuvux Done -\n");
	close_listen_socks();
	unlink(options.pid);
	Free_options();
	exit(EXIT_SUCCESS);
}

/*
 * The main TCP accpet loop
 */
void
server_accept_loop()
{
  fd_set *fdset;
  int maxfd, i, ret, err;
  int newsock;
  struct sockaddr_in from;
  socklen_t fromlen;
  int handcheck = -1;
  int sock;
  char **addr = NULL;
	int pfd[2];

	fdset = NULL;
	maxfd = 0;
	
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, ctr_c);
	signal(SIGQUIT, ctr_c);
	signal(SIGINT, ctr_c);

  /* SSL preliminaries */
  SSL_CTX* ctx;
  SSL*     ssl;
  init_OpenSSL ();			  
  seed_prng ();	

  /* We keep the certificate and key with the context. */
  ctx = setup_server_ctx ();
  
  addr = crv_cut(options.listen_addrs, " ");
  for (i = 0; addr[i]; i++)
  {
		/* Prepare TCP socket for receiving connections (change user for ROOT if port < 1024  )*/
		sock = crv_server_listen(options.num_ports, 5, addr[i], options.address_family, 1);
		if (sock == -1) {
			fprintf( stderr, "%s\n", "main(): server_listent() failed");
			exit (EXIT_FAILURE);
		}
		/* Add comment (listening on several adress )*/
		listen_socks[num_listen_socks] = sock;
		num_listen_socks++;
  }

  for( i = 0; addr[i]; i++)
		crv_free(addr[i]);
  crv_free(addr);

  for (i = 0; i < num_listen_socks; i++)
		if (listen_socks[i] > maxfd)
			maxfd = listen_socks[i];
 
  /* loop */
  for(;;)
  {
		newsock = -1;
		int pid;
		handcheck = -1;

		if (fdset != NULL)
			free(fdset);
	
		fdset = (fd_set *) calloc(howmany(maxfd + 1, NFDBITS), sizeof(fd_mask));

		for ( i = 0; i < num_listen_socks; i++)
			FD_SET(listen_socks[i], fdset);


		/* wait in select until there is a connection. */
		ret = select(maxfd+1, fdset, NULL, NULL, NULL);
		if (ret < 0 && errno != EINTR)  
			Log ( "WARNING", 0, "select: %.100s", strerror(errno));
	
		for ( i = 0; i < num_listen_socks; i++)
			if ( FD_ISSET(listen_socks[i], fdset)) {
				fromlen = sizeof(from);
				newsock = accept(listen_socks[i], (struct sockaddr *)&from, &fromlen);
				if (newsock < 0) {
					if (errno != EINTR && errno != EWOULDBLOCK )
						Log ( "WARNING", 0, "accept: %.100s", strerror(errno));
					continue;
				}

				if (received_sigterm) {
					fprintf( stderr, "Received signal %d; terminating.\n",
						(int) received_sigterm);
					close_listen_socks();
					unlink(options.pid);
					return;
				}
		
				if (crv_unset_nonblock(newsock) == -1) {
					Log ( "CRASH", 0, "Unset nonblock failed");
					continue;
				}
		
				/* TCP connection is ready. Do server side SSL. */
				ssl = SSL_new (ctx);
				if ((ssl)==NULL) {
					Log ( "CRASH", 0, "Create new ssl failed");
					continue;
				}
		
				/* connect the SSL object with a file descriptor */
				err = SSL_set_fd (ssl, newsock);
				if ((err)==0) {
					Log ( "CRASH", 0, "Put SSL on socket failed \n");
					close(newsock);
					continue;
				}

				/* TCP connection is ready. Do server side SSL. */
				err = SSL_accept (ssl);
				if ((err) <= 0) {
					Log ("HACK", 0, "%s handcheck failed", inet_ntoa(from.sin_addr));
					/* Free the allocated SSL structure */
					SSL_free (ssl);
					continue;
				} 
				else {
					handcheck = 0;
				}

				if ( handcheck != -1 )
				{
					/* Pipe creation */
					if (pipe(pfd) == -1)
					{
						fprintf( stderr, "%s\n", "server_accept_loop(): pipe() failed");
						return;
					}

					switch((pid = fork()))
					{
						/* fork() Error */
						case -1:
							Log ("CRASH", 0, "%s%s\n", "Error on fork() -> ", strerror(errno));
							exit (EXIT_FAILURE);
				
						/* First child process */
						case 0:
							/* Close socket that has been created in create_tcp_socket() */
							close_listen_socks();
							int result = 0;
							
							/* Chroot process */
							if (result != -1) 
								result = crv_drop_priv_perm_and_chroot (options.user , options.chroot_directory );

							initialize_command_opt(&command);
							command.addr = crv_strdup((const char *)inet_ntoa (from.sin_addr));
					
							if (options.sec == 1) {
								result = post_connection_check (ssl, command.addr);
								if (result != X509_V_OK) {
									Log ( "HACK", 0, 
									"-Error: peer certificate: %s\n",
									X509_verify_cert_error_string (result));
									result = -1;
								}
							}

							/* Time initialisation */
							gettimeofday (&tv1, NULL);

							/* Get client name*/
							char buff[SIZE];
							command.user = recup_pseudo (ssl);
							command.mail = recup_email (ssl);

							/* Send client pseudo to father */
							write(pfd[1], command.user, strlen(command.user)+1);
							close(pfd[1]); /* close write side */

							sleep(1);
							memset(buff, 0, sizeof(buff));
							
							/* Read repond from father */
							(void)read(pfd[0], buff, SIZE);
							close(pfd[0]);
							
							/* Get father respond about client pseudo */
							/* Les pipes sont à enlever car, la liste des utilisateurs autorisés
							 * sont contenue sur la base de données distante
							if (!crv_strncmp(buff, "no_register")) {
								Log ("WARNING", 0, "Username '%s' is not registered", command.user);
								result = -1;
							}
							*/
							printf("buff:%s\n", buff);
						
							/* Get CMD info (comd name, sha1 file, begin, end) */
							if (result != -1)
								result = pre_authcon( ssl, (const char *)buff);

							if ( result == -1)
								Log ("WARNING", 0, "Can't set command options");
				
							/* exec command give by client */
							if (result != -1)
								traitement_conec ( ssl);
					
							/* Free command structure */
							Free_cmd_struct();
					
							/* Close Socket , tube */
							shutdown (newsock, 2);
							close (newsock);
					
							/* Free the allocated SSL structure */
							SSL_free (ssl);
					
							/* Free the allocated SSL CONTEXT object */
							SSL_CTX_free (ctx);

							/* EXIT */
							exit (EXIT_SUCCESS);
			
						/* Father process */
						default:
							{	
								char *xpath = NULL;
								char Buffer[SIZE];

								/* Read client pseudo from son */
								(void)read(pfd[0], Buffer, SIZE);
								//xpath = get_grp_list(Buffer);
								close(pfd[0]); /* close read side */
								
								if (xpath == NULL)
									xpath = crv_strdup("no_register");
							
								(void)crv_strncpy(Buffer, xpath, sizeof(Buffer));
								(void)write(pfd[1], Buffer, strlen(Buffer)+1);
								close(pfd[1]);
								if (xpath != NULL)
									crv_free(xpath);

								SSL_free (ssl);
								close(newsock);
							}
					} /* End of fork() */

				} /* if handcheck is 0 */

			} /* FD_ISSET */

		} /* End of for() loop */
}

