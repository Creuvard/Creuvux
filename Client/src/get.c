/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

/* SQLite stuff */
#include <sqlite3.h>

#include <pthread.h>

#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "get.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

static off_t File_size;
static char *filename;
static char *sha1;

typedef struct
{
  char *server;
  int port;
  off_t begin;
  char *begin_c;
  off_t end;
  char *end_c;
}about_srv;

static about_srv get_part[20];


void
aff_time (int H, int M, int S, double pct, double xkbps)
{
  if (H < 10)
  {
	if (M < 10)
	{
	  if (S < 10)
	  {
		fprintf (stderr,
		  "\r> Remaining time: 0%d:0%d:0%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
	  else
	  {
	  fprintf (stderr,
		"\r> Remaining time: 0%d:0%d:%d | %.1f%% | %.1f Ko/s",
		H, M, S, pct, xkbps);
	  }
    }
    else
    {
      if (S < 10)
      {
		fprintf (stderr,
		  "\r> Remaining time: 0%d:%d:0%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
      else
	  {
		fprintf (stderr,
		  "\r> Remaining time: 0%d:%d:%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
    }
  }
  else
  {
	if (M < 10)
	{
	  if (S < 10)
	  {
		fprintf (stderr,
		  "\r> Remaining time: %d:0%d:0%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
	  else
	  {
		fprintf (stderr,
		  "\r> Remaining time: %d:0%d:%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
	}
	else
	{
	  if (S < 10)
	  {
		fprintf (stderr,
		  "\r> Remaining time: %d:%d:0%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
	  else
	  {
		fprintf (stderr,
		  "\r> Remaining time: %d:%d:%d | %.1f%% | %.1f Ko/s",
		  H, M, S, pct, xkbps);
	  }
	}
  }
  return;
}

static float xkbps;
static float pct;
static int rate;
static double time_ = 0;
//static int dst;

static void *
receive_get_file (void *arg)
{
  about_srv *get = arg;
  int dst;
  int sd;
  int code;
  int result = -1;
  off_t reste = 0;
  char command[SIZE];
  char file[SIZE];
  char buf[4096];
  SSL_CTX* ctx;
  SSL*     ssl;
  
  /*
  fprintf(stdout, "()->host='%s' port='%d' begin='%s' end='%s' sha1=\"%s\"\n", 
	  get->server,
	  get->port,
	  get->begin_c,
	  get->end_c,
	  sha1);
	*/
  
	int inc = 0;
  /*
  static double xkbps;
  static int rate;
  static double time_ = 0;
  */
  int H = 0, M = 0, S = 0;
  double dl_th = 0;
  //double pct;

  /*
   * If one thread
   */
  if (get->begin == 0) {
		xkbps = 0;
		rate = 0;
  }

  struct timeval tv1, tv2;
  int tops;

	/* fichier a trou */
  (void)crv_strncpy(file, options.tmp_directory, sizeof(file));
  (void)crv_strncat(file, "/", sizeof(file));
  (void)crv_strncat(file, filename, sizeof(file));
  
	dst = open ( file, O_WRONLY | O_NONBLOCK);
  
	if (dst == -1) {
		fprintf (stderr, "%s%s%s%s\n", "receive_get_file(): Err[001] open(", filename,") failed with error ->",
			strerror(errno));
		pthread_exit(NULL);
  }

  if (lseek (dst, get->begin, SEEK_SET) == -1) {
		close(dst);
		fprintf (stderr, "%s%s\n", "receive_get_file(): Err[002] lseek() failed with error ->",
			strerror(errno));
		pthread_exit(NULL);
  }
	reste = File_size;

  /* Chdir in /home/creuvux */
  result = crv_chdir(options.path);
  if (result == -1) {
		close(dst);
		fprintf( stderr, "%s%s%s\n", "receive_get_file(): crv_chdir(", options.path,") failed");
		pthread_exit(NULL);
  }
	
	/* SSL preliminaries */
  init_OpenSSL ();			  
  seed_prng ();
  sd = crv_client_create(get->port, get->server, options.address_family);

  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
		close(dst);close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_sync_file() Err[001] Create new ssl failed");
		pthread_exit(NULL);
  }
		
  /* connect the SSL object with a file descriptor */
  code = SSL_set_fd (ssl, sd);
  if ( code == 0) {
		close(dst); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_sync_file() Err[002] Put SSL on socket failed \n");
		pthread_exit(NULL);
  }

  code = SSL_connect (ssl);
  if (code == 0)	{
		close(dst);close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "receive_sync_file(): Err[003] SSL_connect() failed");
		pthread_exit (NULL);
  } else
  if (code == -1) {
		close(dst);close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "receive_sync_file(): Err[004] SSL_connect() failed");
		pthread_exit (NULL);
  }
  
  /* Change directory for tmp_directory */
  result = crv_chdir(options.tmp_directory);
  if (result == -1) {
		close(dst);close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s%s%s\n", "Get(): crv_chdir(", options.tmp_directory,") failed");
		pthread_exit(NULL);
  }

  /* Build command -> GET#version#sha1#begin#end */
  (void)crv_strncpy(command, "GET#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, get->begin_c, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, get->end_c, sizeof(command));


  code = SSL_write (ssl, command, (int)strlen(command));
  if ( code <= 0) {
		close(dst); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[005] SSL_write() failed.");
		pthread_exit(NULL);
  }

  code = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[code] = '\0';

  tops = sysconf (_SC_CLK_TCK);

  if(!crv_strncmp (buf, "GET_ACK"))
  {
		for(;;)
		{
      if (inc == 0)
				gettimeofday (&tv1, NULL);
			
			code = SSL_read (ssl, buf, sizeof (buf));
			switch (SSL_get_error (ssl, code))
      {
				case SSL_ERROR_NONE:
				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_WRITE:
					break;
				default:
					close(dst); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
					fprintf(stderr, "0)Can't receive FILE\n");
					pthread_exit (NULL);
			}

				
				if (!strncmp (buf, "GET_END", strlen("GET_END"))) {
					break;
				}
					  
				code = write (dst, buf, (size_t)code);
				if (code <= 0) {
					close(dst); close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
					fprintf(stderr, "1)Can't write FILE\n");
					pthread_exit (NULL);
				}
				
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
					time_ = ((double) reste - (double) rate -
						(double) code) / ((double) xkbps * 1000);
					H = M = S = 0;
					H = (int) time_ / 3600;
					M = ((int) time_ - (H * 3600)) / 60;
					S = ((int) time_ - (H * 3600) - (M * 60));
					
					pct = ((double) rate / (double) reste) * 100;
					//aff_time (H, M, S, pct, xkbps);
					fprintf (stderr, "\r> Remaining time: %d h %d min %d sec | %.1f%% | %.1f Ko/s\0", H, M, S, pct, xkbps);
					//dl_progress( (const char *)sha1, (const char *)NULL, (const char *)filename, (float)pct, (float)xkbps);
				}
				rate += code;
			} /* End for(;;) loop */
		}

  close(dst);
  close(sd);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
  pthread_exit(NULL);
}

int Get(const char *crvfile)
{
	THREAD_TYPE Id[10];
	
	FILE *fd;		   /* Open *.crv file */

	char plop[30];
	char tmp[SIZE];
  char final[SIZE];
	char line[1024];
	
	char *Err = 0;
	char **arg = NULL;
	char req[SIZE];

	int rc = -1;
  int i, j;
  int result = -1;
  int file;
	int step = 0;
	long srv_long;
	
	float prct;
  
  step = 0;
  
	memset(plop, 0, sizeof(plop));
	memset(tmp, 0, sizeof(tmp));
	memset(final, 0, sizeof(final));
	memset(line, 0, sizeof(line));

  filename = NULL;
	sha1 = NULL;
  
  struct timeval tv1, tv2;
  int sec1;

	/*
	 * Structur initiation
	 */
	for (i = 0; i < 20; i++) {
		get_part[i].server = NULL;
		get_part[i].port = 0;
		get_part[i].begin = 0;
		get_part[i].begin_c = NULL;
		get_part[i].end = 0;
		get_part[i].end_c = NULL;
  }

  /* Read and get informations about file */
	/*
	 *
	 * Ouvrir le fichier recue via l'interface WEB
	 * pour en sortir le "NOM DU FICHIER", le "SHA1" et la "TAILLE TOTALE"
	 *
	 *
	 */
	fd = crv_fopen(crvfile, "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Missing incoming crv file");
		return (-1);
  }
	j = 0;

	while (fgets(line, sizeof(line), fd) != NULL) {
		line[strcspn(line, "\n")] = '\0';
	
		if (j == 0)
			filename = crv_strdup(line);
		if (j == 1)
			printf("Sha1 => %s\n", line);
		if (j == 2) {
			srv_long = strtol(line, NULL, 10);
			File_size = (off_t)srv_long;
		}

		Err = strstr(line, "/");
		if (Err != NULL) {
			/* cut buf in 2 string. */
			arg = crv_cut (line, "/");
			
			/* For fisrt string we replace the "=" of "CREUVUX_OPT=" with "CREUVUX_OPT\0" */
			for (i = 0; arg[0][i]; i++)
			{
				if (arg[i] != NULL) {
					if (i == 0)	
						get_part[step].server = crv_strdup(arg[i]);
					if (i == 1) {
						srv_long = strtol(arg[i], NULL, 10);
						get_part[step].port = (int)srv_long;
					}
					if (i == 2) {
						get_part[step].begin_c = crv_strdup(arg[i]);
						srv_long = strtol(arg[i], NULL, 10);
						get_part[step].begin = (int)srv_long;
					}
					if (i == 3) {
						get_part[step].end_c = crv_strdup(arg[i]);
						srv_long = strtol(arg[i], NULL, 10);
						get_part[step].end = (int)srv_long;
					}
				}
			}
			step++;
		}
		j++;
	}
	fclose(fd);
	
	/* Chdir in temp directory */
  result = crv_chdir(options.tmp_directory);
  if (result == -1) {
		fprintf( stderr, "%s%s%s\n", "Get(): crv_chdir(", options.tmp_directory,") failed");
		return (-1);
  }

	/* File creation */
  file = open ( filename, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (file <= 0) {
		fprintf( stderr, "%s%s%s%s\n",
			"Get(): Err[001] open(", filename, ") failed with error -> ", 
			strerror(errno));
		return (-1);
  }
 
	/* Allocation for file */
  result = lseek (file, File_size - 1, SEEK_SET);
  if (result == -1) {
		close(file);
		fprintf( stderr, "%s%s\n",
			"Get(): Err[002] lseek() failed with error -> ", 
	  strerror(errno));
		return (-1);
  }
  
  result = write (file, "\0", 1);
  if (result == -1) {
		close(file);
		fprintf( stderr, "%s%s\n",
			"Get(): Err[003] write() failed with error -> ", 
			strerror(errno));
		return (-1);
  }
  close (file);

  fprintf( stdout, 
		"+--------------------------+\n"
		"|\n"
		"| Filename            -> %s\n"
		"| Presence on network -> %d\n"
		"| Size of file        -> %lld\n"
		"|\n"
		"+---------------------------+\n\n",
		filename,
		step,
		File_size);

  for (i = 0; i < step; i++) {
	  
		 fprintf(stdout, "host='%s' port='%d' begin='%s' end='%s'\n", 
											get_part[i].server,
											get_part[i].port,
											get_part[i].begin_c,
											get_part[i].end_c);

		THREAD_CREATE (Id[i], (void *) receive_get_file, get_part[i]);	
	
		sleep(1);
  }
	//return (-1);


  j = i;
  /* Time initialisation */
  gettimeofday (&tv1, NULL);
  
	for (i = 0; i < j; i++) 
		pthread_join (Id[i], NULL);
  
	
	fprintf (stderr, "%s", "\r> Remaining time: 0 h 0 min 0 sec | 100%% | 0 Ko/s\n"); 
	
	(void)crv_strncpy(tmp, options.tmp_directory, sizeof(tmp));
  (void)crv_strncat(tmp, "/", sizeof(tmp));
  (void)crv_strncat(tmp, filename, sizeof(tmp));
  
  (void)crv_strncpy(final, options.download_directory, sizeof(final));
  (void)crv_strncat(final, "/", sizeof(final));
  (void)crv_strncat(final, filename, sizeof(final));
  if (rename(tmp, final))
  {
		if(EXDEV == errno) {
			fprintf(stderr, "%s\n", "Filesystem is different ");
			if (crv_move(tmp, final) == 0) unlink(tmp);
		} else {
			fprintf(stderr, "%s\n", "Move file from tmp directory to download directory failed");
		}
  }

  memset(tmp, 0, sizeof(tmp));
  memset(final, 0, sizeof(final));
	memset(plop, 0, sizeof(plop));
  
  gettimeofday (&tv2, NULL);
  sec1 = tv2.tv_sec - tv1.tv_sec;
  int min;
  float average;
  average = ((float) File_size / (float) sec1) / 1024;
  min = sec1 / 60;
  sec1 = sec1 - min * 60;
  fprintf (stdout,
	     "\n\nFile download in %d min  %d sec (Speed average: %.f Ko/s)\n\n",
	     min, sec1, average);


	if(filename) 
		crv_free(filename);
	
	if(sha1) 
		crv_free(sha1);

  result = crv_chdir(options.path);
  if (result == -1) {
		fprintf( stderr, "%s%s%s\n", "Get(): crv_chdir(", options.tmp_directory,") failed");
		return (-1);
  }
  return (0);
}
