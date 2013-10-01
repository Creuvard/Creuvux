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

#define _XOPEN_SOURCE
#include <time.h> 
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
#include "xml_listing.h"
#include "client_conf.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

#define MAXPRINT 30

/* Client configuration options. */
ClientOptions options;/* Client configuration options. */

/*
 * Xslt transformation.
 */
extern int xmlLoadExtDtDefaultValue;

int 
xsltransform ( const char *xmlfile, const char *resultfile, int flag)
{
	const char *params[1] = { NULL };
	const char sort_style[] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
		"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">"
		"<xsl:output method=\"xml\" version=\"1.0\" encoding=\"ISO-8859-1\" indent=\"yes\" cdata-section-elements=\"title comment\"/>"
		"<xsl:template match=\"/database\">"
		""
		"<database>"
		"<xsl:text>&#xA;</xsl:text>"
		"<xsl:for-each select=\"file\">"
		"<xsl:sort select=\"title\" />"
		"<xsl:copy-of select=\".\"/>"
		"<xsl:text>&#xA;</xsl:text>"
		"</xsl:for-each>"
		""
		"</database>"
		"</xsl:template>"
		"</xsl:stylesheet>";

	const char xsl_style[]= "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
		"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">"
		"<xsl:output method=\"xml\" version=\"1.0\" encoding=\"ISO-8859-1\" indent=\"yes\" cdata-section-elements=\"title comment\"/>"
		"<xsl:key name=\"regrouper\" match=\"file\" use=\"@name\"/>"
		"<xsl:template match=\"/\">"
		"<database>"
		"<xsl:apply-templates select=\"database/file[generate-id(.)=generate-id(key('regrouper', @name)[1])]\"/>"
		"</database>"
		"</xsl:template>"
		"<xsl:template match=\"file\">"
		"<file sha1=\"{@name}\" id=\"{position()}\">"
		"<xsl:apply-templates select=\"title\" />"
		"<xsl:apply-templates select=\"who\" />"
		"<size><xsl:value-of select=\"size\" /></size>"
		/*"<genre><xsl:value-of select=\"genre\" /></genre>"*/
		"<xsl:apply-templates select=\"//server[../@name=current()/@name]\"/>"
		"<xsl:copy-of select=\"about/genre\" />"
		"<xsl:copy-of select=\"about/comment\" />"
		//"<xsl:copy-of select=\"about\" />"
		/*
		"<about>"
		"<genre><xsl:value-of select=\"genre\" /></genre>"
		"<xsl:apply-templates select=\"comment\" />"
		"</about>"
		*/
		"</file>"
		"</xsl:template>"
		""
		"<xsl:template match=\"title\">"
		"<xsl:copy-of select=\".\"/>"
		"</xsl:template>"
		""
		"<xsl:template match=\"comment\">"
		"<xsl:copy-of select=\".\"/>"
		"</xsl:template>"
		""
		"<xsl:template match=\"who\">"
		"<xsl:element name=\"who\">"
		"<xsl:attribute name=\"group\"><xsl:value-of select=\"@group\" /></xsl:attribute>"
		"<xsl:attribute name=\"name\"><xsl:value-of select=\"@name\" /></xsl:attribute>"
		"</xsl:element>"
		"</xsl:template>"
		""
		"<xsl:template match=\"server\">"
		"<xsl:copy-of select=\".\"/>"
		"</xsl:template>"
		"</xsl:stylesheet>";
		

		/* "<xsl:text>&#xA;</xsl:text>" // \n */

	
	xsltStylesheetPtr stylesheet = NULL;
	xmlDocPtr doc, result, xslt = NULL;

	if (flag == 0) {	
		xslt = xmlParseMemory (xsl_style, sizeof(xsl_style));
		if (!xslt) {
			fprintf(stderr, "%s\n", "xsltransform(): Err[001] xmlParseMemory() failed");
			return (-1);
		}
	} else if (flag == 1) {	
		xslt = xmlParseMemory (sort_style, sizeof(sort_style));
		if (!xslt) {
			fprintf(stderr, "%s\n", "xsltransform(): Err[001] xmlParseMemory() failed");
			return (-1);
		}
	}

	xmlSubstituteEntitiesDefault (1); /* Active la substitution des entités */
	xmlLoadExtDtdDefaultValue = 1;    /* Charge les entites externes */

	/* Transformation */
	stylesheet = xsltParseStylesheetDoc (xslt);
	doc = xmlParseFile (xmlfile);
	result = xsltApplyStylesheet (stylesheet, doc, params);

	/* Save file */
	xsltSaveResultToFilename (resultfile, result, stylesheet, 0);

	/* Free use memory */
	xsltFreeStylesheet (stylesheet);
	xmlFreeDoc (result);
	xmlFreeDoc (doc);

	xsltCleanupGlobals ();
	xmlCleanupParser ();
	return (0);
}

int nb_line; /* how many line is printed on list */

/*
 * Use for send message.
 */
int port;
char *srv;


/*
 * File descriptor for WebUI information file.
 */
FILE *info_fd;

/***************************************/
/* DEBUT DE L'UTILISATION DE L'API DOM */
/***************************************/
xmlDocPtr xmldoc;

/* Stock xml_download file in memory */
void download_dom_in (void)
{
	xmldoc = NULL;

	if (NULL == (xmldoc = xmlParseFile ("./listing/listing.xml"))) {
		fprintf(stderr, "%s", "download_dom_in(): Err[001] xmlParseFile() failed\n");
		return;
	}
	
}

/* Drop xml_download memory */
void download_dom_out (void)
{
	xmlFreeDoc (xmldoc);
}


/* Print Head of table */
/* +--------------------+ */
/* | Id | Title | Size  | */
/* +--------------------+ */

static void print_head_of_table (int title_len)
{
	int j;
	/* Print header table   */
	/* ID   -> |--ID--|   6 */
	/* Size -> |--Size--| 8 */
	fprintf( stdout, "%s", "+");
	for (j = 0; j < (title_len + 18); j++)
		fprintf( stdout, "%s", "-");
	fprintf( stdout, "%s", "+\n");

	fprintf( stdout, "%s", "|  Id  |  Title ");
	for (j = 8; j < (title_len + 2); j++)
		fprintf( stdout, "%s", " ");
	fprintf( stdout, "%s", "|  Size  |\n");

	fprintf( stdout, "%s", "+");
	for (j = 0; j < (title_len + 18); j++)
		fprintf( stdout, "%s", "-");
	fprintf( stdout, "%s", "+\n");
}

/* Print bottom of table */
static void print_bottom_of_table (int title_len)
{
	int j;
	/* Print header table   */
	/* ID   -> |--ID--|   6 */
	/* Size -> |--Size--| 8 */
	fprintf( stdout, "%s", "+");
	for (j = 0; j < (title_len + 18); j++)
		fprintf( stdout, "%s", "-");
	fprintf( stdout, "%s", "+\n");
}

/* Print line of table */
static void print_line_of_table(const char *Id, const char *Title, const char *Size, int title_maxlen)
{
	char *ep;
	char mo[SIZE];
	long lval;
	int j;

/* Size operation */
	lval = strtol(Size, &ep, 10);
	if (Size[0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s%s", Size, " is not a number");
		return;
	}
	
	if (lval > 1024*1024*1024)
		(void)snprintf(mo, sizeof(mo), "%ld Go", lval/(1024*1024*1024));
	else if (lval > 1024*1024)
		(void)snprintf(mo, sizeof(mo), "%ld Mo", lval/(1024*1024));
	else 
		(void)snprintf(mo, sizeof(mo), "%ld Ko", lval/(1024));


	/* Print Id cell */
	fprintf(stdout, "%s", "|");
	for (j = 0; j < 5-(int)strlen(Id); j++)
		fprintf(stdout, "%s", " ");
	fprintf(stdout, "%s%s", Id, " | ");

	/* print Title Cell */
	fprintf(stdout, "%s", Title);
	if ( (int)strlen(Title) < title_maxlen) {
		for (j = (int)strlen(Title) ; j < title_maxlen; j++)
			fprintf(stdout, "%s", " ");
	}
	fprintf(stdout, "%s", " | ");

	/* Print Size Cell */
	for (j = 0; j < (6-(int)strlen(mo)); j++)
		fprintf(stdout, "%s", " ");
	fprintf(stdout, "%s%s", mo, " |\n");
	


}

static int lenof(const char *xpath, const char *keyword)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	char *path = NULL;
	int len = -1;
	int tmp = -1;

	path = crv_strdup(xpath);

	/* Recuperation du sha1 et du titre du fichier telecharge */
	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				/* Title */
				if (!crv_strncmp(node->parent->name , keyword) ) {
					tmp = strlen(node->content);
					if (tmp > len)
						len = tmp;
				}
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	return (len);
}


void list (void)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char Id_char[SIZE];
	int Id = 1;
	int title_maxlen = -1;

	title_maxlen = lenof("/database/file/title/text()", "title");

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file/*/text()");
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);


	/* Print header table */
	print_head_of_table (title_maxlen);	


	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				/* Title */
				if (!crv_strncmp(node->parent->name , "title") ) {
					Title = crv_strdup(node->content);
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") ) {
					Size = crv_strdup(node->content);
					(void)snprintf(Id_char, sizeof(Id_char), "%d", Id);
					print_line_of_table(Id_char, Title, Size, title_maxlen);	
					Id++;
					crv_free(Title);
					crv_free(Size);
					memset(Id_char, 0, sizeof(Id_char));
				}
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	print_bottom_of_table (title_maxlen);

}

char *list_categorie_for_upload(void)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	char **Genre;
	char *path = NULL;
	char *ep = NULL;
	char *category = NULL;
	char buf[SIZE];
	int exist = -1;
	int j;
	int id_genre = 0;
	int lval;

	path = crv_strdup("/database/file/genre/text()");
	Genre = calloc(4096 * sizeof *Genre, sizeof(char *));
	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
	 		if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				exist = -1;
				if (id_genre == 0) {
					Genre[id_genre] = crv_strdup( node->content );
					id_genre++;
				}
				for (j=0; Genre[j]; j++) {
					if(!crv_strncmp(node->content, Genre[j]))
						exist = 1;
				}
				if (exist == -1) {
					Genre[id_genre] = strdup( node->content );
					id_genre++;
				}
			}
		}
	}

	couleur ("33;32");
	for (j=0; Genre[j]; j++)
	{
		if (!strstr(Genre[j], "\n"))
		fprintf(stdout, "[%d] %s%s", j, Genre[j], "\n");
	}
	couleur ("0");
	fprintf(stdout, "%s", "Entrez votre choix: ('Entree' si aucun choix ne vous satisfait): ");
	fflush(stdout);
	
	if (fgets(buf, sizeof(buf), stdin) != NULL)
		buf[strcspn(buf, "\n")] = '\0';
	
	lval = (int)strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (j=0; Genre[j]; j++)
			crv_free(Genre[j]);
		crv_free(Genre);
		return NULL;
	}
	if (lval > j) {
		fprintf(stderr, "%s\n", "Out of range");
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (j=0; Genre[j]; j++)
			crv_free(Genre[j]);
		crv_free(Genre);
		return NULL;
	}
	
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);

	category = crv_strdup(Genre[lval]);

	for (j=0; Genre[j]; j++)
		crv_free(Genre[j]);
	crv_free(Genre);

	return (category);
}

void list_categorie(void)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	char **Genre;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char *ep = NULL;
	char Id_char[SIZE];
	char buf[SIZE];
	int Id = 1;
	int title_maxlen = -1;
	int exist = -1;
	int j;
	int id_genre = 0;
	int lval;

	path = crv_strdup("/database/file/genre/text()");
	Genre = calloc(4096 * sizeof *Genre, sizeof(char *));
	
	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				exist = -1;
				if (id_genre == 0) {
					Genre[id_genre] = crv_strdup( node->content );
					id_genre++;
				}
				for (j=0; Genre[j]; j++) {
					if(!crv_strncmp(node->content, Genre[j]))
						exist = 1;
				}
				if (exist == -1) {
					Genre[id_genre] = strdup( node->content );
					id_genre++;
				}
			}
		}
	}

	couleur ("33;32");
	for (j=0; Genre[j]; j++)
	{
		if (!strstr(Genre[j], "\n"))
		fprintf(stdout, "[%d] %s%s", j, Genre[j], "\n");
	}
	couleur ("0");
	fprintf(stdout, "%s", "Entrez votre choix: ('Entree' si aucun choix ne vous satisfait): ");
	fflush(stdout);
	
	if (fgets(buf, sizeof(buf), stdin) != NULL)
		buf[strcspn(buf, "\n")] = '\0';
	
	lval = (int)strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s\n", "Argument is not a number");
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (j=0; Genre[j]; j++)
			crv_free(Genre[j]);
		crv_free(Genre);
		return;
	}
	if (lval > j-1) {
		fprintf(stderr, "%s\n", "Out of range");
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (j=0; Genre[j]; j++)
			crv_free(Genre[j]);
		crv_free(Genre);
		return;
	}
	
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);

	/********************************/
	/* Print list of selected files */
	/********************************/


	title_maxlen = lenof("/database/file/title/text()", "title");

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file/*/text()");
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);


	/* Print header table */
	couleur ("33;32");
	print_head_of_table (title_maxlen);	


	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				/* Title */
				if (!crv_strncmp(node->parent->name , "title") ) {
					Title = crv_strdup(node->content);
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") ) {
					Size = crv_strdup(node->content);
				}

				/* Genre */
				if (!crv_strncmp(node->parent->name , "genre") ) {
					if (!crv_strncmp(Genre[lval], node->content)){
						(void)snprintf(Id_char, sizeof(Id_char), "%d", Id);
						print_line_of_table(Id_char, Title, Size, title_maxlen);	
					}
					Id++;
					crv_free(Title);
					crv_free(Size);
					memset(Id_char, 0, sizeof(Id_char));
				}
			}
		}
	}

	print_bottom_of_table (title_maxlen);
	couleur ("0");
	for (j=0; Genre[j]; j++)
		crv_free(Genre[j]);
	crv_free(Genre);

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}

char *list_group_for_upload(void)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	char **Grp;
	char *group = NULL;
	char *path = NULL;
	char *ep = NULL;
	char buf[SIZE];
	int exist = -1;
	int i;
	int id_genre = 0;
	int lval;

	path = crv_strdup("/database/file/who");
	Grp = calloc(4096 * sizeof *Grp, sizeof(char *));
	
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
			xmlChar *attribut = xmlGetProp(node, "group");
			exist = -1;
			if (id_genre == 0) {
				Grp[id_genre] = crv_strdup( attribut );
				id_genre++;
			}
			for (i=0; Grp[i]; i++) {
				if(!crv_strncmp(attribut, Grp[i]))
					exist = 1;
			}
			if (exist == -1) {
				Grp[id_genre] = strdup( attribut );
				id_genre++;
			}
			xmlFree (attribut);
		}
	}

	couleur ("33;32");
	for (i=0; Grp[i]; i++)
	{
		if (!strstr(Grp[i], "\n"))
		fprintf(stdout, "[%d] %s%s", i, Grp[i], "\n");
	}
	couleur ("0");
	fprintf(stdout, "Entrez votre choix: ('Entree' si aucun choix ne vous satisfait): ");
	fflush(stdout);
	
	if (fgets(buf, sizeof(buf), stdin) != NULL)
		buf[strcspn(buf, "\n")] = '\0';
	
	lval = (int)strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (i=0; Grp[i]; i++)
			crv_free(Grp[i]);
		crv_free(Grp);
		return NULL;
	}

	if (lval > i) {
		fprintf(stderr, "%s\n", "Out of range");
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (i=0; Grp[i]; i++)
			crv_free(Grp[i]);
		crv_free(Grp);
		return NULL;
	}
	
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	
	group = crv_strdup(Grp[lval]);
	
	for (i=0; Grp[i]; i++)
		crv_free(Grp[i]);
	crv_free(Grp);
	return (group);
}






void list_group(void)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	char **Grp;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char *ep = NULL;
	char Id_char[SIZE];
	char buf[SIZE];
	char xpath[SIZE];
	int Id = 0;
	int title_maxlen = -1;
	int exist = -1;
	int i;
	int id_genre = 0;
	int lval;

	path = crv_strdup("/database/file/who");
	Grp = calloc(4096 * sizeof *Grp, sizeof(char *));
	
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
			xmlChar *attribut = xmlGetProp(node, "group");
			exist = -1;
			if (id_genre == 0) {
				Grp[id_genre] = crv_strdup( attribut );
				id_genre++;
			}
			for (i=0; Grp[i]; i++) {
				if(!crv_strncmp(attribut, Grp[i]))
					exist = 1;
			}
			if (exist == -1) {
				Grp[id_genre] = strdup( attribut );
				id_genre++;
			}
			xmlFree (attribut);
		}
	}

	couleur ("33;32");
	for (i=0; Grp[i]; i++)
	{
		if (!strstr(Grp[i], "\n"))
		fprintf(stdout, "[%d] %s%s", i, Grp[i], "\n");
	}
	couleur ("0");
	fprintf(stdout, "Entrez votre choix: ('Entree' si aucun choix ne vous satisfait): ");
	fflush(stdout);
	
	if (fgets(buf, sizeof(buf), stdin) != NULL)
		buf[strcspn(buf, "\n")] = '\0';
	
	lval = (int)strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s\n", "Argument is not a number");
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (i=0; Grp[i]; i++)
			crv_free(Grp[i]);
		crv_free(Grp);
		return;
	}
	if (lval > i-1) {
		fprintf(stderr, "%s\n", "Out of range");
		xmlXPathFreeObject (xmlobject);
		xmlXPathFreeContext (xml_context);
		for (i=0; Grp[i]; i++)
			crv_free(Grp[i]);
		crv_free(Grp);
		return;
	}
	
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);

	/********************************/
	/* Print list of selected files */
	/********************************/
	title_maxlen = lenof("/database/file/title/text()", "title");

	(void)crv_strncpy(xpath, "/database/file[who[@group='", sizeof(xpath));
	(void)crv_strncat(xpath, Grp[lval], sizeof(xpath));
	(void)crv_strncat(xpath, "']]/*/text()", sizeof(xpath));


	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup(xpath);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);


	/* Print header table */
	couleur ("33;32");
	print_head_of_table (title_maxlen);	


	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
  {
		int j;
		xmlNodePtr node;
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
			node = xmlobject->nodesetval->nodeTab[j];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				/* Title */
				if (!crv_strncmp(node->parent->name , "title") ) {
					Title = crv_strdup(node->content);
					Id++;
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") ) {
					Size = crv_strdup(node->content);
					(void)snprintf(Id_char, sizeof(Id_char), "%d", Id);
						print_line_of_table(Id_char, Title, Size, title_maxlen);	
					crv_free(Title);
					crv_free(Size);
					memset(Id_char, 0, sizeof(Id_char));
				}
			}
		}
	}

	print_bottom_of_table (title_maxlen);
	couleur ("0");
	for (i=0; Grp[i]; i++)
		crv_free(Grp[i]);
	crv_free(Grp);

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}

/* Get Info server */
static void get_server_info(const char *ident, long size)
{
	char *path = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	long BP = 0;
	long lval;
	int h, min, sec;
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
			xmlChar *Port = xmlGetProp(node, "port");
			xmlChar *Bandwidth = xmlGetProp(node, "bandwidth");
			lval = strtol(Bandwidth, &ep, 10);
			if (Bandwidth[0] == '\0' || *ep != '\0') {
				fprintf(stderr, "%s%s", Bandwidth, " is not a number");
				return;
			}
			BP += lval;
			fprintf(stdout, "\nServer  : %s:%s (%s Kbps)", Host, Port, Bandwidth);
			xmlFree (Host);
			xmlFree (Port);
			xmlFree (Bandwidth);
		}
	}

	/* Time estimation calcul */
	h = ((size/(1024*BP)/3600));
	min = ((size/(1024*BP)/60)) - (h * 60);
	sec = (size/(1024*BP)) - (min * 60);
	fprintf(stdout, "\nETA     : %d h %d min %d sec", h, min, sec);

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}


/* Get Group from file */
static void get_grp_info(const char *ident)
{
	char *path = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, ident, sizeof(xpath));
	(void)crv_strncat(xpath, "']/who" , sizeof(xpath));
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
			xmlChar *grp = xmlGetProp(node, "group");
			fprintf(stdout, "\nGroup   : %s", grp);
			xmlFree (grp);
		}
	}

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}


/* Info */
void Info(const char *ident)
{
	if (options.debug == 1)
		fprintf(stderr, "On cherche l'id n°%s\n", ident);
	
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;

	char *path = NULL;
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
				fprintf(stdout, "%s%s%s", "\nTitre   : ", node->content, "\n");
			}

			/* Size */
			if (!crv_strncmp(node->parent->name , "size") ) {
				long lval;
				char *ep = NULL;
				char *Size = NULL;
				char mo[SIZE];
				Size = crv_strdup(node->content);
				lval = strtol(Size, &ep, 10);
				if (Size[0] == '\0' || *ep != '\0') {
					fprintf(stderr, "%s%s", Size, " is not a number");
					return;
				}
				crv_free(Size);
				if (lval > 1024*1024*1024)
					(void)snprintf(mo, sizeof(mo), "Taille  : %ld Go", lval/(1024*1024*1024));
				else if (lval > 1024*1024)
					(void)snprintf(mo, sizeof(mo), "Taille  : %ld Mo", lval/(1024*1024));
				else
					(void)snprintf(mo, sizeof(mo), "Taille  : %ld Ko", lval/(1024));
				fprintf(stdout, "%s", mo);
				get_server_info(ident, lval);
				get_grp_info(ident);
			}
			/* Genre */
			if (!crv_strncmp(node->parent->name , "genre") ) {
				fprintf(stdout, "%s%s%s", "\nGenre   : ", node->content, "\n");
			}
			/* Comment */
			if (!crv_strncmp(node->parent->name , "comment") ) {
				fprintf(stdout, "%s%s%s", "Comment : ", node->content, "\n");
			}

		}
	}

		
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}

/* Give number of file */
int return_max_id (void)
{
	char *path = NULL;
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	int nb, result;
	nb = result = -1;

	path = crv_strdup("/database/file");

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
			xmlChar *file_id = xmlGetProp(node, "id");
			nb = (int)strtol(file_id, (char **)NULL, 10);
			if (nb > result )
				result = nb;
			xmlFree (file_id);
		}
	}

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	return (result);
}


void Find(const char *key)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char Id_char[SIZE];
	int Id = 1;
	int title_maxlen = -1;
	int flag = -1;

	title_maxlen = lenof("/database/file/title/text()", "title");

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file/*/text()");
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);


	/* Print header table */
	print_head_of_table (title_maxlen);	


	if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval))
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				/* Title */
				if (!crv_strncmp(node->parent->name , "title") ) {
					char *r = NULL;
					r = crv_strcasestr( node->content, key);
					if (r != NULL) {
						flag = 1;
						Title = crv_strdup(node->content);
					}
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") && (flag == 1)) {
					Size = crv_strdup(node->content);
					(void)snprintf(Id_char, sizeof(Id_char), "%d", Id);
					print_line_of_table(Id_char, Title, Size, title_maxlen);	
					Id++;
					crv_free(Title);
					crv_free(Size);
					memset(Id_char, 0, sizeof(Id_char));
					flag = -1;
				}
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	print_bottom_of_table (title_maxlen);
}


char *Find_id_with_filename(const char *key)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	char *path = NULL;
	char *ID = NULL;
	char xpath[SIZE];

	(void)crv_strncpy(xpath, "/database/file[title/text()[.='", sizeof(xpath));
	(void)crv_strncat(xpath, key, sizeof(xpath));
	(void)crv_strncat(xpath, "']]", sizeof(xpath));

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup(xpath);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		for (j = 0; j < xmlobject->nodesetval->nodeNr; j++)
		{
				node = xmlobject->nodesetval->nodeTab[j];
				xmlChar *xml_id = xmlGetProp(node, "id");
				ID = crv_strdup(xml_id);
				xmlFree (xml_id);
		}
	}

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
	return (ID);
}


