/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


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


#include "creuvard.h"
#include "catls.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

#define MAXPRINT 30

int size_length;
int title_length;

int title_ok;
int genre_ok;
int size_ok;
int nb_line;
int max_print_line;


char *keyword;
char line[SIZE];

static void 
cb_sax_start_document( void *user_data)
{
}

static void 
cb_sax_end_document( void *user_data)
{
}


static void 
cb_sax_start_element_1(void *user_data, const xmlChar * name, const xmlChar ** attributes)
{
	if (!crv_strncmp(name, "title"))
		title_ok = 1;
}

static void 
cb_sax_start_element_2(void *user_data, const xmlChar * name, const xmlChar ** attributes)
{
	if (!crv_strncmp(name, "genre"))
		genre_ok = 1;
	if (!crv_strncmp(name, "title"))
		title_ok = 1;
	if (!crv_strncmp(name, "size"))
		size_ok = 1;
}


static void
cb_sax_end_element(void *user_data, const xmlChar * name)
{
	size_ok = 0;
}

static void
cb_sax_characters_1 (void *user_data, const xmlChar * ch, int len)
{
	if (len > size_length)
		size_length = len;
}

static void
cb_sax_characters_2 (void *user_data, const xmlChar * ch, int len)
{
	if (genre_ok == 1) {
		int i;
		char buf[SIZE];
		for (i = 0; i < len; i++) {
			buf[i] = ch[i];
		}
		buf[i] = '\0';
		if (!crv_strncmp(buf, keyword))
		{
			fprintf(stdout, "%s", line);
			max_print_line++;
			if (max_print_line == MAXPRINT) {
				fprintf(stdout, "Press <Enter> to continue");
				getchar();
				fprintf(stdout, "\r");
				max_print_line = 0;
			}	
			memset(line, 0, sizeof(line));
		}
		genre_ok = 0;
	}

	if(size_ok == 1)
	{
		int i;
			long lval;
			int pad = 0;
			char *ep;
			char Mo[SIZE];
			char buf[SIZE];
			
		  for (i = 0; i < len; i++)
				Mo[i] = ch[i];
			Mo[i]='\0';
			errno = 0;
			lval = strtol(Mo, &ep, 10);
			if (Mo[0] == '\0' || *ep != '\0') {
				fprintf(stderr, "%s%s", Mo, " is not a number");
				return;
			}
			if (lval > 1024*1024) {
				snprintf(Mo, sizeof(Mo), "%ld Mo", lval/(1024*1024));
			}
			else {
				snprintf(Mo, sizeof(Mo), "%ld Ko", lval/1024);
			}
			pad = strlen(Mo);
			for (i = pad; i < size_length; i++)
			  crv_strncat(line, " ", sizeof(line));
			snprintf(buf, sizeof(buf), " %s |\n", Mo);
			crv_strncat(line, buf, sizeof(line));
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
cb_sax_cdata_1 (void *user_data, const xmlChar * ch, int len)
{
  if(title_ok) {
	if (len > title_length)
	  title_length = len;
	title_ok = 0;
  }
	
}

static void
cb_sax_cdata_2 (void *user_data, const xmlChar * ch, int len)
{
	if(title_ok) {
		nb_line++;
		int i;
		char name[SIZE];
		char buf[SIZE];
		for (i = 0; i < len; i++) {
			name[i] = ch[i];
		}
		if (title_length > len) {
			for (i = len; i < title_length; i++) {
			name[i] = ' ';
			}
		}
		name[i] = '\0';
		snprintf( buf, sizeof(buf), "| %4d | %s |", nb_line, name);
		crv_strncpy(line, buf, sizeof(line));
		title_ok = 0;
	}
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
Sax_parse_list (const char *user_data, const char *opt)
{
	xmlParserCtxtPtr ctxt = NULL;
	xmlSAXHandler *sax = NULL;

	sax = crv_malloc(sizeof *sax);
	
	memset(sax, 0, sizeof *sax);

	sax->getEntity = cb_sax_get_entity;
	sax->startDocument = cb_sax_start_document;
	sax->endDocument = cb_sax_end_document;
	
	if (!strncmp(opt, "INIT", strlen("INIT"))) 
	  sax->characters = cb_sax_characters_1;
	else
	if (!strncmp(opt, "PRINT", strlen("PRINT"))) 
		sax->characters = cb_sax_characters_2;
	
	if (!strncmp(opt, "INIT", strlen("INIT")))
	  sax->cdataBlock = cb_sax_cdata_1;
	else
	if (!strncmp(opt, "PRINT", strlen("PRINT")))
	  sax->cdataBlock = cb_sax_cdata_2;
	
	if (!strncmp(opt, "INIT", strlen("INIT")))
		sax->startElement = cb_sax_start_element_1;
	else
	if (!strncmp(opt, "PRINT", strlen("PRINT")))
		sax->startElement = cb_sax_start_element_2;
	
	sax->endElement = cb_sax_end_element;
	sax->comment = cb_sax_comment;
	sax->warning = cb_sax_warning;
	sax->error = cb_sax_error;
	sax->fatalError = cb_sax_fatal_error;

	ctxt = xmlCreateFileParserCtxt ("./listing/listing.xml");
	if( ctxt == NULL )
		return (-1);
	
	ctxt->sax = sax;

	ctxt->userData = (char *)user_data;
	
	xmlParseDocument (ctxt);
	
	/*
	if (ctxt->wellFormed)
		printf("Fichier bien forme\n");
	else
		printf("Fichier a chie\n");
	*/
	ctxt->sax = NULL;
	crv_free(sax);
	xmlFreeParserCtxt (ctxt);
	return (0);
}


int 
catls (const char *search)
{
	int i;
	size_t size = 0;

	size_length = 0;
	title_length = 0;
	title_ok = 0;
	size_ok = 0;
	genre_ok = 0;
	nb_line = 0;
	max_print_line = 0;

	keyword = NULL;
	if (search == NULL) 
		return (-1);
	
	keyword = crv_strdup(search);
	/*
	 * Initialisation values.
	 */
	Sax_parse_list ( NULL, "INIT");


	fprintf(stdout, "%s", "+");
	for (i = 0; i < (title_length + size_length + 12); i++)
		fprintf(stdout, "%s", "-");
	fprintf(stdout, "%s\n", "+");

	fprintf( stdout, "%s", "|");
	fprintf( stdout, "%s", "  Id  |");
	fprintf( stdout, "%s", "  Title ");
	
	size = strlen("  Title ");
	for (i = size; i < (title_length + 2); i++)
		fprintf( stdout, "%s", " ");
	
	size = strlen("| Size");
	fprintf( stdout, "%s", "| Size ");
	
	for (i = size; i < (size_length + 2); i++)
		fprintf( stdout, "%s", " ");
	fprintf( stdout, "%s\n", "|");



	fprintf(stdout, "%s", "+");
	for (i = 0; i < (title_length + size_length + 12); i++)
		fprintf(stdout, "%s", "-");
	fprintf(stdout, "%s\n", "+");

	/*
	 * Use value.
	 */
	  Sax_parse_list ("catls", "PRINT");

	fprintf(stdout, "%s", "+");
	for (i = 0; i < (title_length + size_length + 12); i++)
		fprintf(stdout, "%s", "-");
	fprintf(stdout, "%s\n", "+");	
	
	return(0);
}
