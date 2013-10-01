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
/*
 * XML stuff
 */
#include <libxml/tree.h>
#include <libxml/parserInternals.h>
#include <libxml/xpath.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>


#include <pthread.h>

#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "xml_listing.h"
#include "Gtk_get.h"
#include "Gtk_gui.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

static int step;
static off_t File_size;
static char *filename;
static char *sha1;

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

enum {
    N,
    P,
		Q,
    N_COLUMN
};

static about_srv get_part[20];

typedef struct 
{
	char *Name;
	char *pct;
	char *xkbps;
}about_dl;

static about_dl dl_info[20];

xmlDocPtr xmldoc_dl;
xmlChar **dl_tree;
GtkTreeStore *load_tree;


//static int buffersize;

/* Stock xml_download file in memory */
int Init_download_tree (void)
{
	
	xmldoc_dl = NULL;
	FILE *FD = NULL;
	int fd = -1;
	int flag = -1;
	errno = 0;

	fd = open("./load.xml", O_RDONLY);
	if (fd == -1) {
		if (errno == 2) {
			flag = 1;
		}
		else {
			fprintf(stderr, "%s", "Init_download_tree(): Err[001] open() failed\n");
			return (-1);
		}
	}

	if (flag == 1) {
		FD = crv_fopen("./load.xml", "w");
		if (FD == NULL) {
			fprintf(stderr, "%s", "Init_download_tree(): Err[002] crv_fopen() failed\n");
			return (-1);
		}
		(void)fprintf(FD, "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
			<database><file><title><![CDATA[]]></title><rate>0</rate><pct>0</pct></file></database>");
		(void)fclose(FD);
	}

	if (NULL == (xmldoc_dl = xmlParseFile ("./load.xml"))) {
		fprintf(stderr, "%s", "download_dom_in(): Err[001] xmlParseFile() failed\n");
		return(0);
	}
	//xmlDocDumpFormatMemory(xmldoc_dl, &dl_tree, &buffersize, 1);
	

	/*
	 * Structur initiation
	 */
	/*
	int i;
	for (i = 0; i < 20; i++) {
		dl_info[i].Name = NULL;
		dl_info[i].pct = NULL;
		dl_info[i].xkbps = NULL;
  }
	*/
	return (0);
}

/* Drop xml_download memory */
void Free_download_tree (void)
{
	xmlFreeDoc (xmldoc_dl);
	//xmlFree(dl_tree);
}

void Load(GtkTreeStore *tree)
{
	int i;
	for (i = 0; i < 20; i++) {
		if ((dl_info[i].Name != NULL) && (dl_info[i].pct != NULL) && (dl_info[i].xkbps != NULL))
			fprintf(stdout, "DEBUG_GTK:[%d] %s %s %s\n", i, dl_info[i].Name, dl_info[i].pct, dl_info[i].xkbps);
		else
			continue;
  }
}

void Load1(void)
{
	int i;
	//GtkTreeIter pIter;
	//gtk_tree_store_clear(load_tree);
	for (i = 0; i < 20; i++) {

		if ((dl_info[i].Name != NULL) && (dl_info[i].pct != NULL) && (dl_info[i].xkbps != NULL)) {
			fprintf(stdout, "<DEBUG_GTK>:[%d] %s %s %s\n", i, dl_info[i].Name, dl_info[i].pct, dl_info[i].xkbps);
			//add_file_for_dl (dl_info[i].Name);
			/*
			int u;
			srand(time(NULL));
			u = rand() %10;
			gtk_tree_store_append(load_tree, &pIter, NULL);
			gtk_tree_store_set(load_tree, &pIter, N, dl_info[i].Name, P, dl_info[i].pct, Q, (gdouble)u, -1);
			gtk_widget_show_all(pWindow);
			*/
		}
		else
			continue;
  }
}

void add_file_for_dl (const char *Filename)
{
	int i;
	gtk_notebook_set_current_page (GTK_NOTEBOOK(pNotebook), 1);

	for (i = 0; i < 20; i++) {
		if (dl_info[i].Name == NULL) 
		{	
			dl_info[i].Name = crv_strdup(Filename);
			dl_info[i].pct = crv_strdup("0");
			dl_info[i].xkbps = crv_strdup("0");
			//gtk_tree_store_append(load_tree, &pIter, NULL);
			//gtk_tree_store_set(load_tree, &pIter, N, dl_info[i].Name, P, dl_info[i].xkbps, Q, (gdouble)0, -1);
			break;
		}
  }

	for (i = 0; i < 20; i++) {
		if (dl_info[i].Name != NULL)
			printf("Name(%d): '%s'\n", i, dl_info[i].Name);
	}
		

}

/* Print download and upload progresse in notebook */
static void dl_progress ( const char *Filename, float Pct, float Xkbps)
{
	char *path = NULL;
	//xmlDocPtr mem_xmldoc;
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	
	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc_dl);
	path = crv_strdup("/database/file/ * /text()");
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	int flag = -1;
	char PCT[SIZE];
	char RATE[SIZE];
	snprintf(PCT, sizeof(PCT), "%.0f", Pct);
	snprintf(RATE, sizeof(RATE), "%.0f ", Xkbps);

	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
  {
		int ii;
		xmlNodePtr node;
		for (ii = 0; ii < xmlobject->nodesetval->nodeNr; ii++)
		{
			node = xmlobject->nodesetval->nodeTab[ii];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				if (!crv_strncmp(node->parent->name , "title") ) {
					if (!crv_strncmp(node->content , Filename) ) {
						flag = 1;
					}
				}
				if (!crv_strncmp(node->parent->name , "rate") && (flag == 1) )
					xmlNodeSetContent(node,	RATE);

				if (!crv_strncmp(node->parent->name , "pct") && (flag == 1) )
				{
					xmlNodeSetContent(node,	PCT);
					flag = -1;
				}
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
	xmlXPathFreeContext (xml_context);
	//xmlDocFormatDump(stdout, xmldoc_dl, 1);
	//xmlSaveFileEnc ( "../load.xml", xmldoc_dl, "UTF-8");
	//xmlFreeDoc (mem_xmldoc);
	//Free_download_tree();
	
}








static float xkbps;
static float pct;
static int rate;
static double time_ = 0;
//static int dst;

static void receive_get_file (void *arg)
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
					//fprintf (stderr, "\r> Remaining time: %d h %d min %d sec | %.1f%% | %.1f Ko/s", H, M, S, pct, xkbps);
					//dl_progress( (const char *)filename, (float)pct, (float)xkbps);
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

/* Get Info server */
static void get_server_info(const char *ident)
{
	char *path = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	long lval;
	char *ep;

	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, ident, sizeof(xpath));
	(void)crv_strncat(xpath, "']/server" , sizeof(xpath));
	path = crv_strdup(xpath);

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
			node = xmlobject->nodesetval->nodeTab[j];
			xmlChar *Host = xmlGetProp(node, "host");
			if (Host != NULL) {
				get_part[j].server = crv_strdup(Host);
				step = j+1;
			}

			xmlChar *Port = xmlGetProp(node, "port");
			if (Port != NULL) {
				lval = strtol(Port, &ep, 10);
				if (Port[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s", Port, " is not a number");
					return;
				}
				get_part[j].port = lval;
			}
			
			xmlChar *Bandwidth = xmlGetProp(node, "bandwidth");
			lval = strtol(Bandwidth, &ep, 10);
			if (Bandwidth[0] == '\0' || *ep != '\0') {
				fprintf(stderr, "%s%s", Bandwidth, " is not a number");
				return;
			}
			get_part[j].bandwidth = lval;
			xmlFree (Host);
			xmlFree (Port);
			xmlFree (Bandwidth);
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}

/* Get info about file (Title, Size) */
static int get_info(const char *ident)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	char *path = NULL;
	char *Size = NULL;
	char *ep = NULL;
	char xpath[SIZE];


	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, ident, sizeof(xpath));
	(void)crv_strncat(xpath, "']/*/text()" , sizeof(xpath));
	
	path = crv_strdup(xpath);
	
	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
			node = xmlobject->nodesetval->nodeTab[j];
			/* Title */
			if (!crv_strncmp(node->parent->name , "title") ) {
				filename = crv_strdup(node->content);
			}

			/* Size */
			if (!crv_strncmp(node->parent->name , "size") ) {
				Size = crv_strdup(node->content);
				File_size = strtol(Size, &ep, 10);
				if (Size[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s", Size, " is not a number");
					return (-1);
				}
				crv_free(Size);
				get_server_info(ident);
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	return (0);
}

/* Get sha1file */
static void get_sha1(const char *ident)
{
	char *path = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, ident, sizeof(xpath));
	(void)crv_strncat(xpath, "']" , sizeof(xpath));
	path = crv_strdup(xpath);

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
			node = xmlobject->nodesetval->nodeTab[j];
			xmlChar *Sha1 = xmlGetProp(node, "sha1");
			sha1 = crv_strdup(Sha1);
			xmlFree (Sha1);
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);	
}


int Gtk_get(const char *id)
{
	THREAD_TYPE Id[10];
	char plop[30];
	char tmp[SIZE];
  char final[SIZE];

  int i;
  int result = -1;
  int file;
	
	float prct;
  float bandwidth = 0;
	
	/* File Descriptor use for dowload info in ~/creuvux/Gtk_info/ */
	FILE *fd = NULL;
  
  step = 0;

	//debug();
	//Init_download_tree();
  
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
	result = get_info(id);
	if (result == -1){
		fprintf( stderr, "%s", "Get(): get_info() failed\n");
		return (-1);
	}
	get_sha1(id);

	
	/* Chdir in temp directory */
  result = crv_chdir(options.tmp_directory);
  if (result == -1) {
		fprintf( stderr, "%s%s%s\n", "Get(): crv_chdir(", options.tmp_directory,") failed");
		return (-1);
  }


	//add_file_for_dl (filename);
	/* File creation */
  file = open ( filename, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (file <= 0) {
		fprintf( stderr, "%s%s\n",
			"Get(): Err[001] open() failed with error -> ", 
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
		filename,
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
	
	  /*
		fprintf(stdout, "host='%s' port='%d' bandwidth='%d' begin='%s' end='%s'\n", 
	  get_part[i].server,
	  get_part[i].port,
	  get_part[i].bandwidth,
	  get_part[i].begin_c,
	  get_part[i].end_c);
		*/

		THREAD_CREATE (Id[i], (void *) receive_get_file, get_part[i]);	
	
		sleep(1);
  }
	/* Time initialisation */
  gettimeofday (&tv1, NULL);

	char Gtk_info[SIZE];
	(void)crv_strncpy(Gtk_info, options.gtk_info, sizeof(Gtk_info));
	(void)crv_strncat(Gtk_info, sha1, sizeof(Gtk_info));
	fd = crv_fopen(Gtk_info, "w");
	if (fd == NULL)
	{
		fprintf(stderr, "%s", "Gtk_info creation failed\n");
	}

	while (rate < (int)File_size)
	{
		//fprintf (stderr, "reste=%d, File_size=%d\n", rate, File_size);
		fseek(fd, 0, SEEK_SET);
		//fprintf (stderr, "%s | %.1f%% | %.1f Ko/s\n", filename, pct, xkbps);
		fprintf (fd, "%s#%.1f#%.1f\0", filename, pct, xkbps);
		sleep(1);
	}
	
	



  int j;
  j = i;
  
	for (i = 0; i < j; i++) 
		pthread_join (Id[i], NULL);
	(void)fclose(fd);	
	//Free_download_tree();
	(void)unlink(Gtk_info);
	fd = crv_fopen(Gtk_info, "w");
	if (fd == NULL)
	{
		fprintf(stderr, "%s", "Gtk_info creation failed\n");
	}
	fseek(fd, 0, SEEK_SET);
	fprintf (fd, "%s#100#0\0", filename);
	(void)fclose(fd);
	/* Remove Gtk_info_file ? */
  
	
	fprintf (stderr, "%s", "\r> Remaining time: 0 h 0 min 0 sec | 100\% | 0 Ko/s\n"); 
	float end_pct = 100;
	float end_dl = 0;
	dl_progress( filename, end_pct, end_dl);

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


