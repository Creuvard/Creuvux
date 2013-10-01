/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

extern	SSL		*connect_client (int , const char *, int );
extern	SSL_CTX *setup_client_ctx (void);
extern	int		passwd_cb (char *, int , int , void *);
extern	int		verify_callback (int , X509_STORE_CTX *);
extern	void	init_OpenSSL (void);
extern	void	handle_error (const char *, int , const char *);
extern	void	seed_prng (void);
extern	long	post_connection_check (SSL *, char *);

#endif /* NETWORK_H */
