/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdio.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

/* SSL stuff */
#include <openssl/ssl.h>

/* ZLIB Stuff */
#include <zlib.h>

#include "creuvard.h"
#include "network.h"
#include "server_conf.h"
#include "SSL_loadfile.h"
#include "log.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Server configuration options. */
extern ServerOptions options;


int SSL_loadfile(SSL *ssl, const char *filename)
{
	int result = -1;
	int dst = 0;

	/* Chdir in upload directory */
		result = crv_chdir(options.upload_directory);
		if (result == -1) {
		  Log ("WARNING", 1, 
				"traitement_conec(): crv_chdir(upload_directory) failed");
			return (-1);
		}
	
	result = SSL_write (ssl, "PUT_ACK", (int)strlen("PUT_ACK"));
		if ( result <= 0) {
			Log ("WARNING", 1, 
				"PUT_ACK -> PB");
			return (-1);
		}

	/* Open file with sha1 name */
		dst = open ( (const char *)filename, O_WRONLY | O_CREAT, S_IRWXU);
		if (dst == -1) {
			close(dst);
			Log ("WARNING", 1, 
				"PUT -> Can't create file ");
			return (-1);
		}

		/*********/
		/*  ZLIB */
		/*********/
		/* allocate inflate state */
		int ret = -1;
		unsigned have;
    z_stream strm;
    unsigned char in[SIZE];
    unsigned char out[SIZE];
    
		strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    ret = inflateInit(&strm);
    if (ret != Z_OK) {
			fprintf(stderr, "DEBUG: error inflateInit() = %d\n", ret);
			return ret;
		}

		do {
			strm.avail_in = SSL_read (ssl, in, sizeof (in));
			/*
			fprintf(stderr, "nb_lu = %d\n", strm.avail_in);
			switch (SSL_get_error (ssl, strm.avail_in))
			{
				case SSL_ERROR_NONE:
						fprintf(stderr, "SSL_ERROR_NONE\n");
					break;
				case SSL_ERROR_ZERO_RETURN:
					fprintf(stderr, "SSL_ERROR_ZERO_RETURN\n");
				case SSL_ERROR_WANT_READ:
					fprintf(stderr, "SSL_ERROR_WANT_READ\n");
					break;
				case SSL_ERROR_WANT_WRITE:
					fprintf(stderr, "SSL_ERROR_WANT_WRITE\n");
					break;
				default:
					fprintf(stderr, "KAPOOT\n");
			}
			debug();
			*/
			if (strm.avail_in == 0)
				break;

			strm.next_in = in;
			do {
				strm.avail_out = SIZE;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
				switch (ret) {
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;     /* and fall through */
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						fprintf(stderr, "DEBUG: Z_MEM_ERROR\n");
						close(dst);
						return (-1);
					}
					have = SIZE - strm.avail_out;

					if (!crv_strncmp (out, "PUT_END")) 
						break;
					
					result = write (dst, out, (size_t)have);
					if (result < 0) {
						close(dst);
						result = unlink((const char *)filename);
						if (result == -1)
							Log ("WARNING", 1, 
								"PUT -> Can't delete preload-file(sha1 name) ");
						Log ("WARNING", 1, 
							"PUT -> Can't get file [1]");
						return (-1);
					}
					
			} while (strm.avail_out == 0);
			
		} while (ret != Z_STREAM_END);

		(void)inflateEnd(&strm);


		close(dst);
		result = chmod ( (const char *)filename, S_IRUSR | S_IWUSR);
		if (result == -1) {
			Log ("WARNING", 1, 
				"PUT -> Can't change permissions");
			return (-1);
		}

	return (0);
}
