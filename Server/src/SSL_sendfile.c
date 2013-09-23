/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdio.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

/* SSL stuff */
#include <openssl/ssl.h>

#include "creuvard.h"
#include "network.h"
#include "server_conf.h"
#include "SSL_sendfile.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Server configuration options. */
extern ServerOptions options;

int 
SSL_sendfile(SSL *ssl, const char *filename, off_t begin, off_t end)
{
  long bandwidth = 0;
  struct timeval tv1, tv2;
  off_t rest = 0;
  off_t size = 0;
  size_t nb_read;
  int ssl_writ;
  FILE *fd;
  int i = 0;
  char buf[SIZE];
  float delta;
  float interval;

  bandwidth = options.bandwidth * 1024;
  interval = (float)10 *(float)SIZE / (float)bandwidth;

  /* Time initialisation */
  tv1.tv_sec = tv1.tv_usec = 0;
  tv2.tv_sec = tv2.tv_usec = 0;
  
  /* Check begin and end position */
  size = crv_du (filename);
  
  if (end > size) {
		fprintf( stderr, "%s\n", 
			"SSL_sendfile(): Err[001] Position 'end' "
			"is taller than file system block usage for file ");
		return (-1);
  }
  
  if ( (begin > end) || (end < 0) || (begin < 0)) {
		fprintf( stderr, "%s\n", 
			"SSL_sendfile(): Err[002] Position 'begin' and/or 'end' is false");
		return (-1);
  }

  /* Open file */
  fd = crv_fopen( filename, "rb");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "SSL_sendfile(): Err[003] crv_fopen() failed");
		return (-1);
  }

  /* sets the file position */
  if (fseek(fd, (long) begin, SEEK_SET) != 0) {
		fclose(fd);
		fprintf( stderr, 
			"%s%s\n", 
			"SSL_sendfile(): Err[004] fseek() failed with error -> ",
			strerror(errno));
		return (-1);
  }

  rest = end - begin;
	memset(buf, '\0', sizeof(buf));
  /* Send loop */
  while (rest > 0)
  {
		if (i == 0) 
			gettimeofday (&tv1, NULL);
		i++;

		nb_read = fread(buf, 1, sizeof(buf), fd);
		if (nb_read >= 0) 
		{
			if (i == 10) {
				gettimeofday (&tv2, NULL);
				delta = ((((float) tv2.tv_sec +(float) tv2.tv_usec / 1000000) -
					((float) tv1.tv_sec +(float) tv1.tv_usec / 1000000)));
				if (delta < interval)
					usleep( ((useconds_t) interval - (useconds_t) delta) * 1000000);
				i = 0;
			}
			
			ssl_writ = SSL_write (ssl, buf, (int)nb_read);
			switch (SSL_get_error (ssl, ssl_writ))
			{
				case SSL_ERROR_NONE:
					rest -= ssl_writ;
					break;
				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_WANT_READ:
					break;
				case SSL_ERROR_WANT_WRITE:
					break;
				default:
					fclose(fd);
					fprintf( stderr, "%s\n", "SSL_sendfile(): Err[005] SSL_write() failed");
					return (-1);
			}
		}
  }
  fclose(fd);
  return (0);
}
