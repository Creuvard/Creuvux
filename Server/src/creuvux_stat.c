#include <stdio.h>
#include <string.h>
#include <time.h>



#include "creuvard.h"
#include "network.h"
#include "creuvux_stat.h"
#include "server_conf.h"
#include "help.h"

/* Server configuration options. */
ServerOptions options;

/* Command options. */
Command_options command;

void
Stat ( double size)
{
	time_t t;
  struct tm *tm;
	FILE *fd = NULL;
	char log_date[] = "YYYY/MM/DD HH:MM:SS ";
	t = time (NULL);
  tm = localtime (&t);
  strftime (log_date, sizeof (log_date), "%Y/%m/%d %H:%M:%S", tm);
	
	if ((fd = fopen ("/creuvux.stat", "a")) == NULL) {
		fprintf(stderr, "%s\n", "creuvux_stat.c : Err[OO1] fopen() failed");
		return;
	}
	fprintf( fd, "<action pseudo=\"%s\">\n"\
							 "\t<cmd>%s</cmd>\n"       \
							 "\t<date>%s</date>\n"     \
							 "\t<delta>%.3f</delta>\n" \
							 "\t<size>%lld</size>\n"   \
							 "</action>\n",
							 command.user,
							 command.name,
							 log_date,
							 (float)size,
							 command.end - command.begin);

	fclose(fd);
	return;
}




int size_ok;
int cmd_ok;
int get_flag;
int put_flag;
int sync_flag;
int comment_flag;
int num_user;
int id;

typedef struct 
{
	int nb_sync;
	int nb_put;
	int nb_get;
	int nb_comment;
	int nb_tot;
	off_t size_get;
	off_t size_put;
	off_t size_tot;
	off_t size_comment;
	FILE *fd;
} Gstat;

typedef struct
{
	int nb_sync;
	int nb_put;
	int nb_get;
	int nb_comment;
	int nb_tot;
	off_t size_get;
	off_t size_put;
	off_t size_tot;
	off_t size_comment;
	off_t delta_sync;
	off_t delta_get;
	off_t delta_put;
	int id;
	char *pseudo;
	
} Ustat;

/*
static
void initialize_Ustat ( Ustat *ustat)
{
	ustat->nb_sync = 0;
	ustat->nb_put = 0;
	ustat->nb_get = 0;
	ustat->nb_comment = 0;
	ustat->nb_tot = 0;
	ustat->size_get = 0;
	ustat->size_put = 0;
	ustat->size_tot = 0;
	ustat->size_comment = 0;
	ustat->delta_sync = 0;
	ustat->delta_get = 0;
	ustat->delta_put = 0;
	ustat->id = 0;
	ustat->pseudo = NULL;
}
*/

static
void initialize_Gstat ( Gstat *gstat)
{
  memset(gstat, 0, sizeof(*gstat));
	gstat->nb_sync = 0;
	gstat->nb_put = 0;
	gstat->nb_put = 0;
	gstat->nb_comment = 0;
	gstat->nb_tot = 0;
	gstat->size_get = 0;
	gstat->size_put = 0;
	gstat->size_tot = 0;
	gstat->fd = NULL;
}

Gstat gstat;
Ustat ustat[SIZE];

static
int create_xml_filegen(void)
{
	FILE *fd = NULL;
	FILE *html = NULL;
	char filename[SIZE];
	char buf[SIZE];
	/* STATISTIC ELEMENTS */
	int nb_sync = 0;
	int nb_put = 0;
	int nb_get = 0;
	int nb_comment = 0;

	(void)crv_strncpy(filename, options.chroot_directory, sizeof(filename));
  (void)crv_strncat(filename, "/creuvux.stat", sizeof(filename));

	html = fopen( "creuvux.xml", "w");
	if (html == NULL) {
		fprintf( stderr, "%s\n", "repport_gen(): Err[001] crv_fopen() failed");
		return (-1);
  }

	fd = fopen( filename, "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "repport_gen(): Err[002] crv_fopen() failed");
		return (-1);
  }
	
	fprintf(html, "%s\n", "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>");
	//fprintf(html, "%s\n", "<?xml-stylesheet type=\"text/xsl\" href=\"file.xsl\"?>");
	fprintf(html, "%s\n", "<repport>\n");
	while (fgets(buf, sizeof(buf), fd) != NULL)
  {
		buf[strcspn(buf, "\n")] = '\0';
		fprintf ( html, "%s\n", buf);
		
		if (strstr(buf, "<cmd>PUT</cmd>"))
			nb_put++;
		else if (strstr(buf, "<cmd>GET</cmd>"))
			nb_get++;
		else if (strstr(buf, "<cmd>COMMENT</cmd>"))
			nb_comment++;
		else if (strstr(buf, "<cmd>SYNC</cmd>"))
			nb_sync++;
	}
  if (!feof(fd)) {
		fclose(fd);
		return (-1);
  }
  fclose(fd);
	gstat.nb_sync = nb_sync;
	gstat.nb_get = nb_get;
	gstat.nb_put = nb_put;
	gstat.nb_comment = nb_comment;
	gstat.nb_tot = nb_sync + nb_put + nb_get + nb_comment;
	fprintf(html, "%s\n", "</repport>");
	fclose(html);
	return (0);
}

static void 
cb_sax_start_document( void *user_data)
{
}

static void 
cb_sax_end_document( void *user_data)
{
}


static void 
cb_sax_start_element(void *user_data, const xmlChar * name, const xmlChar ** attributes)
{
	if (!crv_strncmp(name, "size"))
		size_ok = 1;
	if (!crv_strncmp(name, "cmd"))
		cmd_ok = 1;
	if (!crv_strncmp(name, "action"))
	{
		int i;
		int j;
		int exist = -1;
		for ( i = 0; attributes[i]; i += 2)
		{
			if(!crv_strncmp(attributes[i], "pseudo")) {	
				if (num_user == 0) {
					ustat[num_user].pseudo = crv_strdup( attributes[i+1] );
					num_user++;
					id = 0;
				}
				for (j=0; ustat[j].pseudo; j++) {
					if(!crv_strncmp(attributes[i+1], ustat[j].pseudo)) {
						exist = 1; 
						id = j;
					}
				}
				if (exist == -1) {
					ustat[num_user].pseudo = crv_strdup( attributes[i+1] );
					id = num_user;
					num_user++;
				}
				exist = -1;
			}
		}
	}
}

static void
cb_sax_end_element(void *user_data, const xmlChar * name)
{
	size_ok = 0;
	cmd_ok = 0;
}

static void
cb_sax_characters (void *user_data, const xmlChar * ch, int len)
{
	int i;
	if(size_ok) {
		char *ep;
		long lval;
		char octet[SIZE];
		for (i = 0; i < len; i++)
			octet[i] = ch[i];
		octet[i]='\0';
		lval = strtol(octet, &ep, 10);
		if (octet[0] == '\0' || *ep != '\0') {
			fprintf(stderr, "%s%s", octet, " is not a number");
			return;
		}
		gstat.size_tot += lval;
		if (put_flag) {
			ustat[id].nb_put++;
			ustat[id].size_put += lval;
			gstat.size_put += lval;
			put_flag = 0;
		}
		else if (get_flag) {
			ustat[id].nb_get++;
			ustat[id].size_get += lval;
			gstat.size_get += lval;
			get_flag = 0;
		}
		else if (comment_flag) {
			ustat[id].nb_comment++;
			ustat[id].size_comment += lval;
			gstat.size_comment += lval;
			comment_flag = 0;
		}
		else if (sync_flag) {
			ustat[id].nb_sync++;
			sync_flag = 0;
		}
	}
	if(cmd_ok){
		char cmd[SIZE];
		for (i = 0; i < len; i++)
			cmd[i] = ch[i];
		cmd[i]='\0';
		if (!crv_strncmp(cmd, "GET"))
			get_flag = 1;
		else if (!crv_strncmp(cmd, "PUT"))
			put_flag = 1;
		else if (!crv_strncmp(cmd, "COMMENT"))
			comment_flag = 1;
		else if (!crv_strncmp(cmd, "SYNC"))
			sync_flag = 1;
	}
}

static xmlEntityPtr
cb_sax_get_entity ( void *user_data, const xmlChar *name)
{
	return xmlGetPredefinedEntity (name);
}

static void cb_sax_comment (void *user_data, const xmlChar * ch)
{
}

static void
cb_sax_cdata (void *user_data, const xmlChar * ch, int len)
{
}

static void
cb_sax_warning (void *user_data, const char *msg, ...)
{
}

static void
cb_sax_error(void *user_data, const char *msg, ...)
{
}

static void
cb_sax_fatal_error(void *user_data, const char *msg, ...)
{
}


static int
genrepport (const char *user_data)
{
	xmlParserCtxtPtr ctxt = NULL;
	xmlSAXHandler *sax = NULL;

	sax = malloc(sizeof *sax);
	
	memset(sax, 0, sizeof *sax);

	sax->getEntity = cb_sax_get_entity;
	sax->startDocument = cb_sax_start_document;
	sax->endDocument = cb_sax_end_document;
	sax->characters = cb_sax_characters;
	sax->cdataBlock = cb_sax_cdata;
	sax->startElement = cb_sax_start_element;
	sax->endElement = cb_sax_end_element;
	sax->comment = cb_sax_comment;
	sax->warning = cb_sax_warning;
	sax->error = cb_sax_error;
	sax->fatalError = cb_sax_fatal_error;

	ctxt = xmlCreateFileParserCtxt ("./creuvux.xml");
	if( ctxt == NULL )
		return (-1);
	
	ctxt->sax = sax;

	ctxt->userData = (char *)user_data;
	
	xmlParseDocument (ctxt);
	
	if (ctxt->wellFormed)
		printf("Fichier bien forme\n");
	else
		printf("Fichier a chie\n");
	ctxt->sax = NULL;
	free(sax);
	xmlFreeParserCtxt (ctxt);
	return (0);
}

int
Repport(void )
{
	time_t t;
  struct tm *tm;
  char log_date[] = "YYYY/MM/DD HH:MM:SS ";
	int i;
	char filename[SIZE];

	(void)crv_strncpy(filename, options.chroot_directory, sizeof(filename));
  (void)crv_strncat(filename, "/repport.xml", sizeof(filename));

	num_user = 0;

  t = time (NULL);
  tm = localtime (&t);
  strftime (log_date, sizeof (log_date), "%Y/%m/%d %H:%M:%S", tm);
	size_ok = 0;

	initialize_Gstat (&gstat);
	gstat.fd = fopen( filename, "w");
  if (gstat.fd == NULL) {
		fprintf( stderr, "%s\n", "main(): Err[001] fopen() failed");
		return (-1);
  }
	create_xml_filegen();
	genrepport(NULL);
	
	fprintf(gstat.fd, "%s\n", "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>");
	fprintf(gstat.fd, "%s\n", "<?xml-stylesheet type=\"text/xsl\" href=\"xslt.xsl\"?>");
	fprintf(gstat.fd, "%s\n", "<repport>\n");
	fprintf(gstat.fd, "<version>%s</version>\n", CREUVUX_VERSION);
	fprintf(gstat.fd, "<create>%s</create>\n", log_date);
	/* SYNC */
	fprintf(gstat.fd, "<nb_sync>%d</nb_sync>\n<nb_sync_pct>%.1f</nb_sync_pct>\n", gstat.nb_sync, (float)gstat.nb_sync/(float)gstat.nb_tot * 100);
	fprintf(stdout, "Nb SYNC:%d (%.1f\%%)\n", gstat.nb_sync, (float)gstat.nb_sync/(float)gstat.nb_tot * 100);
	/* GET */
	fprintf(gstat.fd, "<nb_get>%d</nb_get>\n<nb_get_pct>%.1f</nb_get_pct>\n", gstat.nb_get, (float)gstat.nb_get/(float)gstat.nb_tot * 100);
	fprintf(stdout, "Nb GET:%d (%.1f\%%)\n", gstat.nb_get, (float)gstat.nb_get/(float)gstat.nb_tot * 100);

	/* PUT */
	fprintf(gstat.fd, "<nb_put>%d</nb_put>\n<nb_put_pct>%.1f</nb_put_pct>\n", gstat.nb_put, (float)gstat.nb_put/(float)gstat.nb_tot * 100);
	fprintf(stdout, "Nb PUT:%d (%.1f\%%)\n", gstat.nb_put, (float)gstat.nb_put/(float)gstat.nb_tot * 100);
	
	/* COMMENT */
	fprintf(gstat.fd, "<nb_comment>%d</nb_comment>\n<nb_comment_pct>%.1f</nb_comment_pct>\n", gstat.nb_comment, (float)gstat.nb_comment/(float)gstat.nb_tot * 100);
	fprintf(stdout, "Nb COMMENT:%d (%.1f\%%)\n", gstat.nb_comment, (float)gstat.nb_comment/(float)gstat.nb_tot * 100);
	
	/* TOTAL */
	fprintf(gstat.fd, "<nb_tot>%d</nb_tot>\n", gstat.nb_tot);
	fprintf(stdout, "Nb Con:%d\n", gstat.nb_tot);

	/* TOTAL */
	if ( gstat.size_tot > 1024 *1024 *1024) {
		fprintf(gstat.fd, "<size_tot len=\"Go\">%.2f</size_tot>\n",	(float)gstat.size_tot /(float)(1024 *1024 *1024));
		fprintf(stdout, "Quantite de donnee echangees: %.2f Go\n",	(float)gstat.size_tot /(float)(1024 *1024 *1024));
	} else if ( gstat.size_tot > 1024 *1024) {
		fprintf(gstat.fd, "<size_tot len=\"Mo\">%lld</size_tot>\n",	gstat.size_tot /(1024 *1024));
		fprintf(stdout, "Quantite de donnee echangees: %lld Mo\n",	gstat.size_tot /(1024 *1024));
	} else {
		fprintf(gstat.fd, "<size_tot len=\"Ko\">%lld</size_tot>\n",	gstat.size_tot/1024);
		fprintf(stdout, "Quantite de donnee echangees: %lld Ko\n",	gstat.size_tot /(1024 ));
	}
	
	/* GET */
	if ( gstat.size_get > 1024 *1024 *1024) {
		fprintf(gstat.fd, "<size_get len=\"Go\">%.2f</size_get>\n",	(float)gstat.size_get /(float)(1024 *1024 *1024));
		fprintf(stdout, "Quantite de donnee envoyees: %lld Go\n",	gstat.size_get /(1024 *1024 *1024));
	} else if ( gstat.size_get > 1024 *1024) {
		fprintf(gstat.fd, "<size_get len=\"Mo\">%lld</size_get>\n",	gstat.size_get /(1024 *1024));
		fprintf(stdout, "Quantite de donnee envoyees: %lld Mo\n",	gstat.size_get /(1024 *1024));

	} else {
		fprintf(gstat.fd, "<size_get len=\"Ko\">%lld</size_get>\n",	gstat.size_get/1024);
		fprintf(stdout, "Quantite de donnee envoyees: %lld Ko\n",	gstat.size_get /(1024));
	}

	fprintf(gstat.fd, "<size_get_pct>%.1f</size_get_pct>\n",((float)gstat.size_get/(float)gstat.size_tot)*100);
	
	
	/* PUT */
	if ( gstat.size_put > 1024 *1024 *1024) {
		fprintf(gstat.fd, "<size_put len=\"Go\">%.2f</size_put>\n",	(float)gstat.size_put /(float)(1024 *1024 *1024));
		fprintf(stdout, "Quantite de donnee recues: %lld Go\n",	gstat.size_tot /(1024 *1024 *1024));
	} else if ( gstat.size_put > 1024 *1024) {
		fprintf(gstat.fd, "<size_put len=\"Mo\">%lld</size_put>\n",	gstat.size_put /(1024 *1024));
		fprintf(stdout, "Quantite de donnee recues: %lld Mo\n",	gstat.size_tot /(1024 *1024));
	} else {
		fprintf(gstat.fd, "<size_put len=\"Ko\">%lld</size_put>\n",	gstat.size_put/1024);
		fprintf(stdout, "Quantite de donnee recues: %lld Ko\n",	gstat.size_tot /(1024));
	}
	fprintf(gstat.fd, "<size_put_pct>%.1f</size_put_pct>\n",((float)gstat.size_put/(float)gstat.size_tot)*100);
	
	/* COMMENT */
	if ( gstat.size_comment > 1024 *1024 *1024)
		fprintf(gstat.fd, "<size_comment len=\"Go\">%.1f</size_comment>\n",	(float)gstat.size_comment /(float)(1024 *1024 *1024));
	else if ( gstat.size_comment > 1024 *1024)
		fprintf(gstat.fd, "<size_comment len=\"Mo\">%lld</size_comment>\n",	gstat.size_comment /(1024 *1024));
	else 
		fprintf(gstat.fd, "<size_comment len=\"Ko\">%lld</size_comment>\n",	gstat.size_comment/1024);
	fprintf(gstat.fd, "<size_comment_pct>%.1f</size_comment_pct>\n",	((float)gstat.size_comment/(float)gstat.size_tot)*100);


	printf("num_user = %d\n", num_user);
	for (i = 0; i < num_user; i++) {
		fprintf(stdout, "User: '%s'\n", ustat[i].pseudo);
		fprintf(gstat.fd, "<user name=\"%s\">\n", ustat[i].pseudo);
		fprintf(gstat.fd, "<total>%d</total>\n", 
			ustat[i].nb_sync+ustat[i].nb_put+ustat[i].nb_get+ustat[i].nb_comment);
		fprintf(stdout, "\t|- Nb con: %d\n", 
			ustat[i].nb_sync+ustat[i].nb_put+ustat[i].nb_get+ustat[i].nb_comment);
		/* BEGIN SYNC */
		fprintf(gstat.fd, "<nb_sync>%d</nb_sync>\n", ustat[i].nb_sync);
		fprintf( stdout, "\t|- Nb SYNC: %d\n", ustat[i].nb_sync);
		fprintf(gstat.fd, "<size_sync_pct>%.1f</size_sync_pct>\n",((float)ustat[i].nb_sync/(float)(ustat[i].nb_sync+ustat[i].nb_put+ustat[i].nb_get+ustat[i].nb_comment))*100);
		/* END SYNC */
		
		/* BEGIN PUT */
		fprintf(gstat.fd, "<nb_put>%d</nb_put>\n", ustat[i].nb_put);
		fprintf(stdout, "\t|-Nb PUT:%d\n", ustat[i].nb_put);
		
		if ( ustat[i].size_put > 1024 *1024 *1024) {
			fprintf(gstat.fd, "<size_put len=\"Go\">%.2f</size_put>\n",	(float)ustat[i].size_put /(float)(1024 *1024 *1024));
			fprintf(stdout, "\t`-Quantite de donnee recues:%.2f Go\n",	(float)ustat[i].size_put /(float)(1024 *1024 *1024));
		} else if ( ustat[i].size_put > 1024 *1024) {
			fprintf(gstat.fd, "<size_put len=\"Mo\">%lld</size_put>\n",	ustat[i].size_put /(1024 *1024));
			fprintf(stdout, "\t`-Quantite de donnee recues:%lld Mo\n",	ustat[i].size_put /(1024 *1024));
		} else {
			fprintf(gstat.fd, "<size_put len=\"Ko\">%lld</size_put>\n",	ustat[i].size_put/1024);
			fprintf(stdout, "\t`-Quantite de donnee recues:%lld Ko\n",	ustat[i].size_put /(1024));
		}
		fprintf(gstat.fd, "<size_put_pct>%.1f</size_put_pct>\n",((float)ustat[i].nb_put/(float)(ustat[i].nb_sync+ustat[i].nb_put+ustat[i].nb_get+ustat[i].nb_comment))*100);
		/* END PUT */

		/* BEGIN GET */
		fprintf(gstat.fd, "<nb_get>%d</nb_get>\n", ustat[i].nb_get);
		fprintf(stdout, "\t|- Nb GET:%d\n", ustat[i].nb_get);
		
		if ( ustat[i].size_get > 1024 *1024 *1024) {
			fprintf(gstat.fd, "<size_get len=\"Go\">%.2f</size_get>\n",	(float)ustat[i].size_get /(float)(1024 *1024 *1024));
			fprintf(stdout, "\t`-Quantite de donnee telechargees:%.2f Go\n",	(float)ustat[i].size_get /(float)(1024 *1024 *1024));
		} else if ( ustat[i].size_get > 1024 *1024) {
			fprintf(gstat.fd, "<size_get len=\"Mo\">%lld</size_get>\n",	ustat[i].size_get /(1024 *1024));
			fprintf(stdout, "\t`-Quantite de donnee telechargees:%lld Mo\n",	ustat[i].size_get /(1024 *1024));
		} else {
			fprintf(gstat.fd, "<size_get len=\"Ko\">%lld</size_get>\n",	ustat[i].size_get/1024);
			fprintf(stdout, "\t`-Quantite de donnee telechargees:%lld Ko\n",	ustat[i].size_get /(1024));
		}
		fprintf(gstat.fd, "<size_get_pct>%.1f</size_get_pct>\n",((float)ustat[i].nb_get/(float)(ustat[i].nb_sync+ustat[i].nb_put+ustat[i].nb_get+ustat[i].nb_comment))*100);
		/* END GET */
		
		/* BEGIN COMMENT */
		fprintf(gstat.fd, "<nb_comment>%d</nb_comment>\n", ustat[i].nb_comment);
		
		if ( ustat[i].size_comment > 1024 *1024 *1024)
			fprintf(gstat.fd, "<size_comment len=\"Go\">%.2f</size_comment>\n",	(float)ustat[i].size_comment /(float)(1024 *1024 *1024));
		else if ( ustat[i].size_comment > 1024 *1024)
			fprintf(gstat.fd, "<size_comment len=\"Mo\">%lld</size_comment>\n",	ustat[i].size_comment /(1024 *1024));
		else 
			fprintf(gstat.fd, "<size_comment len=\"Ko\">%lld</size_comment>\n",	ustat[i].size_comment/1024);
		fprintf(gstat.fd, "<size_comment_pct>%.1f</size_comment_pct>\n",((float)ustat[i].nb_comment/(float)(ustat[i].nb_sync+ustat[i].nb_put+ustat[i].nb_get+ustat[i].nb_comment))*100);
		/* END COMMENT */
		fprintf(gstat.fd, "</user>\n");
	}
	fprintf(gstat.fd, "%s\n", "</repport>\n");
	
	fclose(gstat.fd);
	return (0);
}
