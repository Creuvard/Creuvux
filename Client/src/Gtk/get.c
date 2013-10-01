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

static int step;
static off_t File_size;
static char *filename;
static char *sha1;
static FILE *fd_info;

typedef struct
{
  char *server;
  int port;
  int bandwidth;
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
	char *Title = NULL;
  SSL_CTX* ctx;
  SSL*     ssl;
  
	Title = crv_strdup(filename);
  /*
  fprintf(stdout, "()->host='%s' port='%d' bandwidth='%d' begin='%s' end='%s' sha1=\"%s\"\n", 
	  get->server,
	  get->port,
	  get->bandwidth,
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
  (void)crv_strncat(file, Title, sizeof(file));
  
	dst = open ( file, O_WRONLY | O_NONBLOCK);
  
	if (dst == -1) {
		fprintf (stderr, "%s%s%s%s\n", "receive_get_file(): Err[001] open(", Title,") failed with error ->",
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
					
					if (options.gui == 0)
						fprintf (stderr, "\r> Remaining time: %d h %d min %d sec | %.1f%% | %.1f Ko/s", H, M, S, pct, xkbps);
					else if (options.gui == 1) {
						fseek(fd_info, 0, SEEK_SET);
						fprintf (fd_info, "%s#%.1f#%.1f\0", Title, pct, xkbps);
					}

					//fprintf (stderr, "\r> Remaining time: %d h %d min %d sec | %.1f%% | %.1f Ko/s\0", H, M, S, pct, xkbps);
				}
				rate += code;
			} /* End for(;;) loop */
		}

  close(dst);
  close(sd);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
	crv_free(Title);
  pthread_exit(NULL);
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
		if (!crv_strncmp(azColName[i], "Size"))
			File_size = (off_t)atoi(argv[i]);
		if (!crv_strncmp(azColName[i], "Sha1"))
			sha1 = crv_strdup(argv[i]);
		if (!crv_strncmp(azColName[i], "Name"))
			filename = crv_strdup(argv[i]);
  }
  return 0;
}

static int get_srv_info(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
		if (!crv_strncmp(azColName[i], "Port"))
			get_part[step].port = (off_t)atoi(argv[i]);
		if (!crv_strncmp(azColName[i], "bandwidth"))
			get_part[step].bandwidth = (off_t)atoi(argv[i]);
		if (!crv_strncmp(azColName[i], "Servername"))
			get_part[step].server = crv_strdup(argv[i]);
  }
	step += 1;
  return 0;
}


int Get(sqlite3 *db, const char *id)
{
	THREAD_TYPE Id[10];
	char plop[30];
	char tmp[SIZE];
  char final[SIZE];
	char *zErrMsg = 0;
	char *req = NULL;
	char *Title = NULL;

	int rc = -1;
  int i;
  int result = -1;
  int file;
	
	float prct;
  float bandwidth = 0;
	fd_info = NULL;

  
  step = 0;
  
	memset(plop, 0, sizeof(plop));
	memset(tmp, 0, sizeof(tmp));
	memset(final, 0, sizeof(final));

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
		get_part[i].bandwidth = 0;
		get_part[i].begin = 0;
		get_part[i].begin_c = 0;
		get_part[i].end = 0;
		get_part[i].end_c = 0;
  }

  /* Read and get informations about file */
	req = sqlite3_mprintf("SELECT DISTINCT Files.Name,Files.Sha1,Files.Size FROM Files WHERE Files.rowid='%q';", id);
	rc = sqlite3_exec(db, req, callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);
	
	if(sha1 == NULL) {
		fprintf( stderr, "%s\n", "Check yout ID");
		return (-1);
	}
	Title = crv_strdup(filename);

	if (options.gui == 1) {
		char Gtk_info[SIZE];
		(void)crv_strncpy(Gtk_info, options.path, sizeof(Gtk_info));
		(void)crv_strncat(Gtk_info, "Gtk_info/", sizeof(Gtk_info));
		(void)crv_strncat(Gtk_info, sha1, sizeof(Gtk_info));
		
		fd_info = crv_fopen(Gtk_info, "w");
		if (fd_info == NULL)
		{
			fprintf(stderr, "%s", "Gtk_info creation failed\n");
		}
		
	}
	
	/* Read and get informations about server(s) */
	req = sqlite3_mprintf("SELECT Server.Servername,Server.Port,Server.bandwidth FROM Server WHERE Server.Sha1='%q';", sha1);
	rc = sqlite3_exec(db, req, get_srv_info, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	sqlite3_free(req);
	
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

  
	for (i = 0; i < step; i++) {
		bandwidth += get_part[i].bandwidth;
  }

  fprintf( stdout, 
		"+--------------------------+\n"
		"|\n"
		"| Filename            -> %s\n"
		"| Presence on network -> %d\n"
		"| Size of file        -> %lld\n"
		"|\n"
		"+---------------------------+\n\n",
		Title,
		step,
		File_size);

  for (i = 0; i < step; i++) {
		prct = 0;
		prct = (float)get_part[i].bandwidth / bandwidth;
		if (i == 0 ) {
			get_part[i].begin = 0;
			get_part[i].end = prct * File_size;
			snprintf(plop, sizeof(plop), "%lld", get_part[i].begin);
			get_part[i].begin_c = crv_strdup(plop);
			snprintf(plop, sizeof(plop), "%lld", get_part[i].end);
			get_part[i].end_c = crv_strdup(plop);
		}
		else {
			get_part[i].begin = get_part[i-1].end;
			get_part[i].end = get_part[i-1].end + prct * File_size;
			snprintf(plop, sizeof(plop), "%lld", get_part[i].begin);
			get_part[i].begin_c = crv_strdup(plop);
			snprintf(plop, sizeof(plop), "%lld", get_part[i].end);
			get_part[i].end_c = crv_strdup(plop);
		}
	
	  
		 fprintf(stdout, "host='%s' port='%d' bandwidth='%d' begin='%s' end='%s'\n", 
											get_part[i].server,
											get_part[i].port,
											get_part[i].bandwidth,
											get_part[i].begin_c,
											get_part[i].end_c);

		THREAD_CREATE (Id[i], (void *) receive_get_file, get_part[i]);	
	
		sleep(1);
  }
  int j;
  j = i;
  /* Time initialisation */
  gettimeofday (&tv1, NULL);
  
	for (i = 0; i < j; i++) 
		pthread_join (Id[i], NULL);
 
	if (options.gui == 0)
		fprintf (stderr, "%s", "\r> Remaining time: 0 h 0 min 0 sec | 100%% | 0 Ko/s\n");
  else if (options.gui == 1) {
		fseek(fd_info, 0, SEEK_SET);
		fprintf (fd_info, "%s#100#0\0", Title);
	}

	if (options.gui == 1)
		fclose(fd_info);

	(void)crv_strncpy(tmp, options.tmp_directory, sizeof(tmp));
  (void)crv_strncat(tmp, "/", sizeof(tmp));
  (void)crv_strncat(tmp, Title, sizeof(tmp));
  
  (void)crv_strncpy(final, options.download_directory, sizeof(final));
  (void)crv_strncat(final, "/", sizeof(final));
  (void)crv_strncat(final, Title, sizeof(final));
  if (rename(tmp, final))
  {
		if(EXDEV == errno) {
			fprintf(stderr, "%s\n", "Filesystem is different ");
			if (crv_move(tmp, final) == 0) unlink(tmp);
		} else {
			fprintf(stderr, "rename('%s', '%s')\n", tmp, final);
			fprintf(stderr, "%s\n", "Move file from tmp directory to download directory failed");
			fprintf(stderr, "%s\n", strerror(errno));
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

	/*
	if(filename != NULL) 
		crv_free(filename);

	if (Title != NULL)
		crv_free(Title);
	
	if(sha1 != NULL) 
		crv_free(sha1);
	*/


  result = crv_chdir(options.path);
  if (result == -1) {
		fprintf( stderr, "%s%s%s\n", "Get(): crv_chdir(", options.tmp_directory,") failed");
		return (-1);
  }
  return (0);
}
