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

#include "creuvard.h"
#include "client_conf.h"
#include "creuvux_upload.h"


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

FILE *output;
int title_flag;
int eta_flag;
int pct_flag;
int rate_flag;
int ok_flag;
int never;

char *sha1;
char *title;
char *eta;
double pct;
double rate;

static void 
cb_sax_start_document( void *user_data)
{
	fprintf(output, "%s", "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	fprintf(output, "%s", "<?xml-stylesheet type=\"text/xsl\" href=\"/web/up.xsl\"?>\n");
	fprintf(output, "%s", "<download>\n");
}

static void 
cb_sax_end_document( void *user_data)
{
	if (never == 0)
	{
		fprintf(output, "<file sha1=\"%s\">\n", sha1);
		fprintf(output, "<title><![CDATA[%s]]></title>\n", title );
		fprintf(output, "<eta>%s</eta>\n", eta );
		fprintf(output, "<pct>%.2f</pct>\n", pct);
		fprintf(output, "<rate>%.2f</rate>\n", rate);
		fprintf(output, "%s", "</file>\n");
	}
	fprintf(output, "%s", "</download>\n");
}

static void 
cb_sax_start_element(void *user_data, const xmlChar * name, const xmlChar ** attributes)
{
	if(!crv_strncmp(name, "file"))
	{
		int i;
		for (i = 0; attributes[i]; i+= 2)
		{
			if(!crv_strncmp(attributes[i], "sha1")) {
				if(!crv_strncmp(attributes[i+1], sha1)) {
					fprintf(output, "<file sha1=\"%s\">\n", sha1);
					fprintf(output, "<title><![CDATA[%s]]></title>\n", title );
					fprintf(output, "<eta>%s</eta>\n", eta );
					fprintf(output, "<pct>%.2f</pct>\n", pct);
					fprintf(output, "<rate>%.2f</rate>\n", rate);
					ok_flag = 0;
					never = 1;
				} else {
					fprintf(output, "<file sha1=\"%s\">\n", attributes[i+1]);
					ok_flag = 1;
				}
			}
		}
	}

	/*
	 * TITLE
	 */
	if(!crv_strncmp(name, "title") && (ok_flag))
		title_flag = 1;

	/*
	 * ETA
	 */
	if(!crv_strncmp(name, "eta") && (ok_flag))
		eta_flag = 1;
	
	/*
	 * PCT
	 */
	if(!crv_strncmp(name, "pct") && (ok_flag))
		pct_flag = 1;
	
	/*
	 * RATE
	 */
	if(!crv_strncmp(name, "rate") && (ok_flag))
		rate_flag = 1;
}

static void
cb_sax_end_element(void *user_data, const xmlChar * name)
{
	if(!crv_strncmp(name, "file"))
		fprintf(output, "%s", "</file>\n" );
}

static void
cb_sax_characters (void *user_data, const xmlChar * ch, int len)
{
	/*
	 * ETA
	 */
	if(eta_flag) {
		int i;
		char buf[len];
		for (i = 0; i < len; i++)
			buf[i] = ch[i];
		buf[len] = '\0';
		fprintf(output, "%s%s%s", "<eta>", buf, "</eta>\n");
		memset(buf, 0, sizeof(buf));
		eta_flag = 0;
  }
	
	/*
	 * PCT
	 */
	if(pct_flag) {
		int i;
		char buf[len];
		for (i = 0; i < len; i++)
			buf[i] = ch[i];
		buf[len] = '\0';
		fprintf(output, "%s%s%s", "<pct>", buf, "</pct>\n");
		memset(buf, 0, sizeof(buf));
		pct_flag = 0;
  }

	/*
	 * RATE
	 */
	if(rate_flag) {
		int i;
		char buf[len];
		for (i = 0; i < len; i++)
			buf[i] = ch[i];
		buf[len] = '\0';
		fprintf(output, "%s%s%s", "<rate>", buf, "</rate>\n");
		memset(buf, 0, sizeof(buf));
		rate_flag = 0;
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
	/*
	 * TITLE
	 */
	if(title_flag) {
		int i;
		char buf[len];
		for (i = 0; i < len; i++)
			buf[i] = ch[i];
		buf[len] = '\0';
		fprintf(output, "%s%s%s", "<title><![CDATA[", buf, "]]></title>\n");
		memset(buf, 0, sizeof(buf));
		title_flag = 0;
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
Sax_parse_list (const char *file)
{
	xmlParserCtxtPtr ctxt;
	xmlSAXHandler *sax;

	sax = crv_malloc(sizeof *sax);
	
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
	ctxt = xmlCreateFileParserCtxt (file);
	if( ctxt == NULL )
		return (-1);
	
	ctxt->sax = sax;
	ctxt->userData = (char *)NULL;
	if( ctxt != NULL )	
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
upload_history(const char *sha1_dl, const char *eta_dl, const char *title_dl, double pct_dl, double rate_dl)
{
	int result = -1;
	char path[SIZE];
	char repport[SIZE];
	output = NULL;
	eta_flag = 0;
	pct_flag = 0;
	rate_flag = 0;
	ok_flag = 0;
	never = 0;

	/***********/
	sha1 = NULL;
	title = NULL;
	eta = NULL;
	pct = 0;
	rate = 0;
	/***********/

	memset(path, 0, strlen(path));
	memset(repport, 0, strlen(repport));

	(void)crv_strncpy(path, options.path, sizeof(path));
	(void)crv_strncat(path, "web/", sizeof(path));
	(void)crv_strncat(path, "tmp_upload.xml", sizeof(path));

	(void)crv_strncpy(repport, options.path, sizeof(repport));
	(void)crv_strncat(repport, "web/", sizeof(repport));
	(void)crv_strncat(repport, "upload.xml", sizeof(repport));

	output = crv_fopen( path, "w");
	if (output == NULL) {
		fprintf(stderr, "WebUI_dl() : Err[001] crv_fopen() failed");
		return (-1);
	}

	if(sha1_dl)
		sha1 = crv_strdup((const char *)sha1_dl);
	if (eta_dl)
		eta = strdup(eta_dl);
	if (title_dl)
		title = crv_strdup(title_dl);
	
	pct = pct_dl;
	rate = rate_dl;

	Sax_parse_list (repport);
	
	if (sha1)
		crv_free(sha1);
	if (title)
		crv_free(title);
	if (eta)
		crv_free(eta);
	
	fclose(output);
	
	result = unlink(repport);
	if (result == -1) {
		fprintf(stderr, "WebUI_dl() : Err[002] unlink() failed with eror -> %s ", strerror(errno));
		return (-1);
	}
	result = rename( path, repport);
	if (result == -1) {
		fprintf(stderr, "WebUI_dl() : Err[003] rename() failed with eror -> %s ", strerror(errno));
		return (-1);
	}

	return (0);
}
