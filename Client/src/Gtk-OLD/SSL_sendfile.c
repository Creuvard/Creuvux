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

#include <assert.h>
#include <zlib.h>

/* SSL stuff */
#include <openssl/ssl.h>

#include "creuvard.h"
#include "network.h"
#include "client_conf.h"
#include "get.h"
#include "SSL_sendfile.h"
#include "creuvux_upload.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
extern ClientOptions options;



#define SIZE 1024
#define LEVEL 9



/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}




int 
SSL_sendfile(SSL *ssl, const char *sha1, const char *filename, off_t begin, off_t end)
{
  off_t rest = 0;
  off_t size = 0;
  size_t nb_read;
  int ssl_writ;
  FILE *fd;
  char buf[SIZE];
	unsigned char in[SIZE];
  unsigned char out[SIZE];

  /* ETA */
  int inc = 0;
  double xkbps;
  int rate;
  double time_ = 0;
  int H = 0, M = 0, S = 0;
  double dl_th = 0;
  double pct;
  xkbps = 0;
  rate = 0;
  struct timeval tv1, tv2;

	int ret = -1; 
	int flush = -1;
	unsigned have;
	z_stream strm;


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
	
	
	/* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
	ret = deflateInit(&strm, LEVEL);
  if (ret != Z_OK) {
		fclose(fd);
		zerr(ret);
		return (-1);
	}

	/*************/
	/* SEND LOOP */
	/*************/	

	do {
				if (inc == 0)
					gettimeofday (&tv1, NULL);
        
				strm.avail_in = fread(in, 1, SIZE, fd);
        
				if (ferror(fd)) {
            (void)deflateEnd(&strm);
						fclose(fd);
            return Z_ERRNO;
        }
        
				flush = feof(fd) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;


        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
						
						strm.avail_out = SIZE;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = SIZE - strm.avail_out;
						if (have != 0) {
							ssl_writ = SSL_write (ssl, out, sizeof(out) - strm.avail_out);
							rest -= ssl_writ;
				 		  rate += ssl_writ;
						}
						
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

				inc++;
	  
					/* Calcul and print bitrate */
					
					if (inc == 100)
					{
						gettimeofday (&tv2, NULL);
						xkbps -= dl_th;
						dl_th = 100 /
							( ((double) tv2.tv_sec + (double) tv2.tv_usec / 1000000) -
							((double) tv1.tv_sec + (double) tv1.tv_usec / 1000000));
						inc = 0;
						xkbps += dl_th;
						time_ = ((double) size - (double) rate -
							(double) ssl_writ) / ((double) xkbps * 1000);
						H = M = S = 0;
						H = (int) time_ / 3600;
						M = ((int) time_ - (H * 3600)) / 60;
						S = ((int) time_ - (H * 3600) - (M * 60));
						pct = ((double) rate / (double) size) * 100;
						fprintf (stderr, "\r> Remaining time: %d h %d min %d sec | %.1f%% | %.1f Ko/s", H, M, S, pct, xkbps);
					}
					

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

		(void)deflateEnd(&strm);
	/************/
	/* END LOOP */
	/************/
	
	fprintf (stderr, "%s", "\r> Remaining time: 0 h 0 min 0 sec | 100%% | 0 Ko/s\n");
  fclose(fd);
  return (0);
}
