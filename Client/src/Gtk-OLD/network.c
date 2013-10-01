/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>



#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>

/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>


#include "creuvard.h"
#include "network.h"
#include "client_conf.h"

#define int_error(msg)  do { handle_error(__FILE__, __LINE__, msg); exit(EXIT_FAILURE); } while (0)

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;



void
seed_prng (void)
{
  RAND_load_file ("/dev/urandom", 1024);
}


void
handle_error (const char *file, int lineno, const char *msg)
{
  fprintf (stderr, "%s %d %s\n", file, lineno, msg);
  ERR_print_errors_fp (stderr);
}


/*
 * Initialisation OpenSSL
 */
void
init_OpenSSL (void)
{
  if (!SSL_library_init ())
    {
      fprintf (stderr, "%s\n", "** OpenSSL initialization failed!");
      exit (EXIT_FAILURE);
    }
  SSL_load_error_strings ();
}

int
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


int
passwd_cb (char *buf, int size, int rwflag, void *passwd)
{
  strncpy (buf, (char *) (options.passphrase), (size_t) size);
  buf[size - 1] = '\0';
  return (strlen (options.passphrase));
}

#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"
#define CAFILE "rootcert.pem"
#define CADIR NULL
#define CERTFILE "client.pem"
SSL_CTX *
setup_client_ctx (void)
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
  /*
  SSL_CTX_set_tmp_dh_callback (ctx, tmp_dh_callback);
  
  if (SSL_CTX_set_cipher_list (ctx, CIPHER_LIST) != 1)
    int_error ("ERREUR DE LECTURE DE LA LISTE DE CHIPTER");
  */
  return ctx;
}


/*
 * Connect client to the SSL server.
 */

SSL *
connect_client (int port, const char *addr, int addr_family)
{
  int sd;
  int err;
  int result = -1;
  /* SSL preliminaries */
  SSL_CTX* ctx;
  SSL*     ssl;
  init_OpenSSL ();			  
  seed_prng ();
  
  sd = crv_client_create(port, addr, addr_family);
  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
	fprintf ( stderr, "%s\n", "Create new ssl failed");
  }
		
  /* connect the SSL object with a file descriptor */
  err = SSL_set_fd (ssl, sd);
  if ((err)==0) {
	fprintf ( stderr, "%s\n", "Put SSL on socket failed \n");
	close(sd);
  }

  result = SSL_connect (ssl);
  if (result == 0)	{
	SSL_get_error(ssl, result);
	return (NULL);
  } else
  if (result == -1) {
	fprintf( stderr, "%s\n", "client_(): Err[003] SSL_connect() failed");
	return (NULL);
  }
  return (ssl);
}

long
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
		       && !strcmp (nval->value, host)) || (!strcmp (nval->name,
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


