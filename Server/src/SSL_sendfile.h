/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#ifndef SSL_SENDFILE_H
#define SSL_SENDFILE_H
#include <openssl/ssl.h>

extern int SSL_sendfile(SSL *, const char *, off_t , off_t );

#endif /* SSL_SENDFILE_H */
