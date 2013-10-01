/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 * This program is the creuvux daemon.  It listens for connections from clients,
 * and performs authentication, executes use commands, and forwards
 * information to/from the application to the user client over an encrypted
 * connection.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <pwd.h>
#include <libgen.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

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

/*
 * Glib/Gtk+-2.0
 */
#include <gtk/gtk.h>
#include <glib/gprintf.h>


#include "creuvard.h"
#include "client_conf.h"
#include "help.h"
#include "sync.h"
#include "xml_listing.h"
#include "msg.h"
#include "catls.h"
#include "thread.h"
#include "Gtk_get.h"
#include "Gtk_gui.h"
#include "Gtk_put.h"



#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;


/* DEBUT DE LA MODIFICATION POUR GCREUVUX */


/*
 * Element a afficher
 */
char **genre;
char **groupe;
char *cat;
int num_genre;
int num_groupe;
int print_flag;
char *choice_in_list;
int download_this_on;
char *info_title;
char *info_cat;
char *info_comment;
char *info_size;
char *info;

/*
 * Divers Flag
 */
int num;
int size_ok;
int title_ok;
int genre_ok;
int comment_ok;





/*
 * La liste des icones se trouve dans le repertoire
 * /usr/local/include/gtk-2.0/gtk/gtkstock.h
 */

enum {
    N,
    P,
		Q,
    N_COLUMN
};

GtkWidget		*pWindow;

static GtkTreeStore *pTreeStore;
static GtkWidget		*pTreeView;
static GtkWidget		*pStatusBar;
static GtkWidget		*List_HBox;
static GtkWidget		*ListBox;
GtkWidget		*pNotebook;

//GtkTreeStore *load_tree;
static GtkTreeSelection *load_selection;
static GtkWidget		*load_view;


static void List(void)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	GtkTreeIter pIter;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char Id_char[SIZE];
	int Id = 1;

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file/*/text()");
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
				if (!crv_strncmp(node->parent->name , "title") ) {
					Title = crv_strdup(node->content);
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") ) {
					char *ep = NULL;
					char mo[SIZE];
					long lval;
					Size = crv_strdup(node->content);
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
					
					(void)snprintf(Id_char, sizeof(Id_char), "%d", Id);
					gtk_tree_store_append(pTreeStore, &pIter, NULL);
					gtk_tree_store_set(pTreeStore, &pIter, N, Id_char, P, Title, Q, mo, -1);
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
}

/* Genere la liste des differents groupes */
static void gen_grp_list(void)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	char *path = NULL;
	path = crv_strdup("/database/file/who");
	int exist = -1;
	int i;
	num_groupe = 0;
	groupe = calloc(4096 * sizeof *groupe, sizeof(char *));

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
			if (num_groupe == 0) {
				groupe[num_groupe] = crv_strdup( attribut );
				num_groupe++;
			}
			for (i=0; groupe[i]; i++) {
				if(!crv_strncmp(attribut, groupe[i]))
					exist = 1;
			}
			if (exist == -1) {
				groupe[num_groupe] = strdup( attribut );
				num_groupe++;
			}
			xmlFree (attribut);
		}
	}

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}

/* Genere la liste deroulante */
static void gen_combo_box_list(void)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	char *path = NULL;
	path = crv_strdup("/database/file/genre/text()");
	int exist = -1;
	int i;
	num_genre = 0;
	genre = calloc(4096 * sizeof *genre, sizeof(char *));

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);

	if ((xmlobject->type == XPATH_NODESET) )
  {
		int j;
		xmlNodePtr node;
		for (i = 0; i < xmlobject->nodesetval->nodeNr; i++)
		{
			node = xmlobject->nodesetval->nodeTab[i];
			if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
			{
				exist = -1;
				if (num_genre == 0) {
					genre[num_genre] = crv_strdup( node->content );
					num_genre++;
				}
				for (j=0; genre[j]; j++) {
					if(!crv_strncmp(node->content, genre[j]))
						exist = 1;
				}
				if (exist == -1) {
					genre[num_genre] = strdup( node->content );
					num_genre++;
				}
			}
		}
	}

	gtk_combo_box_insert_text(GTK_COMBO_BOX(ListBox), 0, "All");
	for (i=0; genre[i]; i++)
	{
		if (!strstr(genre[i], "\n")) {
			gtk_combo_box_insert_text(GTK_COMBO_BOX(ListBox), i+1, genre[i]);
		}
	}

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}


static void select_cat(GtkWidget *widget, gpointer data)
{
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	GtkTreeIter pIter;

	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char mo[SIZE];
	char Id_char[SIZE];
	int j;
	int NB = 1;

  j = gtk_combo_box_get_active(GTK_COMBO_BOX(data));
	gtk_tree_store_clear(pTreeStore);
	//fprintf(stderr, "Choix dans la liste = %d\n", j);
	if (j==0)
	{
		debug();
		List();
		return;
	}
	//fprintf(stderr, "Nom de la categorie --> %s\n", genre[j-1]);
	if (genre[j-1] == NULL)
		return;
	

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file/*/text()");
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
				if (!crv_strncmp(node->parent->name , "title") ) {
					Title = crv_strdup(node->content);
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") ) {
					Size = crv_strdup(node->content);
					char *ep = NULL;
					long lval;
					Size = crv_strdup(node->content);
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
				}

				/* Genre */
				if (!crv_strncmp(node->parent->name , "genre") ) {
					if (!crv_strncmp(genre[j-1], node->content) ){
						(void)snprintf(Id_char, sizeof(Id_char), "%d", NB);
						gtk_tree_store_append(pTreeStore, &pIter, NULL);
						gtk_tree_store_set(pTreeStore, &pIter, N, Id_char, P, Title, Q, mo, -1);
					}
					NB++;
					crv_free(Title);
					crv_free(Size);
					memset(Id_char, 0, sizeof(Id_char));
				}
			}
		}
	}

	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
}

static void get_selected(GtkWidget *widget, gpointer statusbar)
{
	GtkTreeIter iter;
  GtkTreeModel *model;
  char *value;
	char *id_value;
  if (gtk_tree_selection_get_selected(
      GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, P, &value,  -1);
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),
			gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), value), value);

		gtk_tree_model_get(model, &iter, N, &id_value,  -1);
		if (choice_in_list != NULL) {
			g_free(choice_in_list);
			choice_in_list = strdup(id_value);
		} else choice_in_list = strdup(id_value);

		fprintf(stderr, "Choix => %s\n", value);
		g_free(value);
		g_free(id_value);
  }
}



static void sync_boutton (GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	int result = -1;
	int i;

	print_flag = 0;
	
	/* Nettoyage de la liste */
	gtk_tree_store_clear(pTreeStore);

	gtk_statusbar_push(GTK_STATUSBAR(pStatusBar),
        gtk_statusbar_get_context_id(GTK_STATUSBAR(pStatusBar), 
            "Synchronisation en cours ..."), "Synchronisation en cours ...");
	result = Sync();
	if (result == 0){
		gtk_statusbar_push(GTK_STATUSBAR(pStatusBar),
        gtk_statusbar_get_context_id(GTK_STATUSBAR(pStatusBar), 
            "Synchronisation OK."), "Synchronisation OK.");
	}

	/* Libere le tableau des genres. */
	for (i = 0; genre[i]; i++)
		crv_free(genre[i]);
	crv_free(genre);

	/* Enleve les categories de la combox box */
	gtk_combo_box_remove_text(GTK_COMBO_BOX(ListBox), 0);
	for (i=0; genre[i]; i++)
	{
		if (!strstr(genre[i], "\n"))
			gtk_combo_box_remove_text(GTK_COMBO_BOX(ListBox), 0);
	}

	/* Rempli la liste deroulante avec les categories*/
	gen_combo_box_list();

	download_dom_out();
	download_dom_in();
	/* Rempli la liste dans l'onglet qui va bien*/
	List();
}


/* Update progress bar in download_tree */
/* Read all file in ~/creuvux/Gtk_info/ */
/* and print infos found in each file   */
/* 1 file equal 1 line on load_tree     */
static void Print_load_progress(void)
{
		fprintf(stderr, " ");
		
		DIR *dirp = NULL;
		struct dirent *dp;
		FILE *fd = NULL;
	char path[SIZE];
	char buf[SIZE];
	char **a;
		int i;
		gdouble pct;
	GtkTreeIter pIter;

	gtk_tree_store_clear(GTK_TREE_MODEL(load_tree));
		
		dirp = opendir(options.gtk_info);
		if (dirp == NULL)
			return;
   
		while ((dp = readdir (dirp))) 
		{
			
			if (!crv_strncmp(dp->d_name, "."))
			continue;
		else if (!crv_strncmp(dp->d_name, ".."))
			continue;
		else
		{
			(void)crv_strncpy(path, options.gtk_info, sizeof(path));
			(void)crv_strncat(path, dp->d_name , sizeof(path));
			
			fd = crv_fopen(path, "r");
			if (fd == NULL ) {
				(void)closedir(dirp);
				return;
			}
			
			fgets(buf, sizeof(buf), fd);
			if (strlen(buf) > 0) 
			{
				char Bp[SIZE];
				a = crv_cut(buf, "#");
				if (a[0] && a[1] && a[2]) 
				{
					pct = atoi(a[1]);
					(void)crv_strncpy(Bp, a[2], sizeof(Bp));
					(void)crv_strncat(Bp, " Ko/s", sizeof(Bp));
					//fprintf(stderr," '%s' '%s' '%d'\n", a[0], Bp, pct);
					gtk_tree_store_append(GTK_TREE_MODEL(load_tree), &pIter, NULL);
					gtk_tree_store_set(GTK_TREE_MODEL(load_tree), &pIter, N, a[0], P, Bp, Q, (gdouble)pct, -1);
				}
				memset(Bp, 0, sizeof(Bp));
				for ( i = 0; a[i]; i++)
					crv_free(a[i]);
				crv_free(a);
			}
			
			fclose(fd);
		}
		}

		(void)closedir(dirp);
		fprintf(stderr, " ");
		
		return;
}

/*
static void Print_load_progress (void)
{
	DIR *FD = NULL;
	FILE *fd = NULL;
	struct dirent *f;
	char path[SIZE];
	char buf[SIZE];
	char **a;
	double pct;
	int i;
	GtkTreeIter pIter;

	gtk_tree_store_clear(load_tree);


	FD = opendir (options.gtk_info);
	if (NULL == FD)
	{
		fprintf(stderr, "%s%s%s", "Print_load_progress(): Err[001] opendir(", path, ") failed\n");
		return;
	}

	while ((f = readdir (FD)))
	{
		fd = NULL;
		pct = 0;
		memset(buf, 0, sizeof(buf));
		memset(path, 0, sizeof(path));


		if (!crv_strncmp(f->d_name, "."))
			continue;
			//fprintf(stderr, "On a le fichier '.'\n");
		else if (!crv_strncmp(f->d_name, ".."))
			continue;
			//fprintf(stderr, "On a le fichier '..'\n");
		else
		{
			(void)crv_strncpy(path, options.path, sizeof(path));
			(void)crv_strncat(path, "Gtk_info/", sizeof(path));
			(void)crv_strncat(path, f->d_name , sizeof(path));
			
			
			fd = crv_fopen(path, "r");
			if (fd == NULL ) {
				fprintf(stderr, "%s%s%s", "Print_load_progress(): Err[002] crv_fopen(", path, ") failed\n");
			}
			
			fgets(buf, sizeof(buf), fd);
			if (strlen(buf) > 0) 
			{
				char Bp[SIZE];
				a = crv_cut(buf, "#");
				if (a[1])
					pct = atoi(a[1]);
				
				(void)crv_strncpy(Bp, a[2], sizeof(Bp));
				(void)crv_strncat(Bp, " Ko/s", sizeof(Bp));
				
				gtk_tree_store_append(load_tree, &pIter, NULL);
				gtk_tree_store_set(load_tree, &pIter, N, a[0], P, Bp, Q, (gdouble)pct, -1);
	
				memset(Bp, 0, sizeof(Bp));
				for ( i = 0; a[i]; i++)
					crv_free(a[i]);
				crv_free(a);
			}
			
			fclose(fd);
		}
	}  
	closedir(FD);
	return;
}
*/


static void close_tab (GtkWidget *pWidget, gpointer filename)
{
	gint page;
	page = gtk_notebook_get_current_page (GTK_NOTEBOOK(pNotebook));
	gtk_notebook_remove_page (GTK_NOTEBOOK(pNotebook), page);
	//gtk_notebook_insert_page(GTK_NOTEBOOK(pNotebook), pLabel1, download_tab, 1);
}


static void Gtk_download(GtkWidget *pWidget, gpointer id)
{
	int  pid;
	fprintf(stderr, "%s", "Dans le proc parent\n");
	char *path = NULL;
	char *Titre = NULL;
	char xpath[SIZE];
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	
	fprintf(stderr, "gpointer = %s\n", (char *)id);
	
	//path = crv_strdup("/database/file/*/text()");
	
	/* Recuperation du sha1 et du titre du fichier telecharge */
	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, choice_in_list, sizeof(xpath));
	(void)crv_strncat(xpath, "']/title/text()" , sizeof(xpath));
	path = crv_strdup(xpath);
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
				Titre = crv_strdup(node->content);
			}
		}
	}
	xmlXPathFreeObject (xmlobject);
	xmlXPathFreeContext (xml_context);
	
	
	if (Titre != NULL) crv_free(Titre);
	
	if ((pid = fork()) < 0)
	{
		printf("fork failed\n");
		return;
	}
	
	if (pid == 0)
	{
		/* child */
		//g_timeout_add_seconds ((guint)1, (GSourceFunc)print_load_progress, NULL);
		Gtk_get(id);
		if(id != NULL)
			crv_free(id);
		exit(EXIT_SUCCESS);
	}
	else
	{
	}
}



/* Ferme le widget passe en parametre */
static void Close_widget(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
}


/* Parametre pour la selection de fichier */
static GtkWidget *path_label;
static gchar *sChemin;
static GtkWidget *cat_choix1;
static GtkWidget *cat_choix2;
static GtkWidget *grp_choix1;
static GtkWidget *grp_choix2;
static GtkWidget *cat_list_combobox;
static GtkWidget *grp_list_combobox;
static GtkWidget *srv_list_combobox;
static GtkWidget *cat_entry;
static GtkWidget *grp_entry;
static GtkWidget *pTextView;
static GtkWidget *up_win;


static void Warning (const gchar *msg)
{
	GtkWidget *pAbout;

    /* Creation de la boite de message */
    /* Type : Information -> GTK_MESSAGE_INFO */
    /* Bouton : 1 OK -> GTK_BUTTONS_OK */
    pAbout = gtk_message_dialog_new (GTK_WINDOW(NULL),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK,
        "Avertissement:\n%s",
        msg);

    /* Affichage de la boite de message */
    gtk_dialog_run(GTK_DIALOG(pAbout));

    /* Destruction de la boite de message */
    gtk_widget_destroy(pAbout);
}


/* Fonction qui verifie que tout les parametres sont entres */
/* afin avant de lancer l'upload sur le serveur distant     */
static void Valid_upload(GtkWidget *widget, gpointer data)
{
	GSList *pList;
	const gchar *sLabel = NULL;
	char *grp = NULL;
	char *categorie = NULL;
	char *server = NULL;
	char Description[SIZE];
	FILE *fd = NULL;
	char *sha1 = NULL;
	char *truename = NULL;
	char *tmp = NULL;
	
	if (sChemin == NULL) {
		Warning ("\nFichier manquant !");
		return;
	}

	/************************/
	/* Recuperation serveur */
	/************************/
	
	int k = -1;
	k = gtk_combo_box_get_active(GTK_COMBO_BOX(srv_list_combobox));
	if (k != -1) {
		int i;
		FILE *fd_ = NULL;
		char path[SIZE];
		char buf[SIZE];
		
		(void)crv_strncpy(path, options.path, sizeof(path));
		(void)crv_strncat(path, "serveur_liste", sizeof(path));
		
		fd_ = crv_fopen(path, "r");
		if (fd_ == NULL) {
			Warning("Lecture du fichier\nserveur_liste impossible");
			return;
		}
		i = 0;
		while(fgets(buf, sizeof(buf), fd_) != NULL)
		{
			buf[strcspn(buf, "\n")] = '\0';
			if (i == k)
				server = strdup(buf);
			i++;
		}
		fclose(fd_);
	}

	/***************/
	/* Fin serveur */
	/***************/
	/********************************/
	/* Recuperation de la categorie */
	/********************************/
	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(cat_choix1));
	/* Parcours de la liste */
  while(pList)
  {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data)))
    {
        sLabel = gtk_button_get_label(GTK_BUTTON(pList->data));
        pList = NULL;
    }
    else
    {
        pList = g_slist_next(pList);
    }
  }
	
	if (!crv_strncmp("Categorie existante: ", sLabel)) {
		/* On recupere la categorie choisis dans la combobox*/
		int j = -1;
		j = gtk_combo_box_get_active(GTK_COMBO_BOX(cat_list_combobox));
		if (j != -1)
			categorie = strdup(genre[j]);
	} else
	if (!crv_strncmp("Choix nouvelle categorie : ", sLabel)) {
		/* On recupere la nouvelle categorie dans la zone de texte */
		const gchar *sText;
		sText = gtk_entry_get_text(GTK_ENTRY(cat_entry));
		if (strlen(sText) == 0) {
			Warning ("\nCategorie manquante !");
			return;
		}
		categorie = strdup(sText);
	}
	/*******************************/
	/* Fin de la section CATEGORIE */
	/*******************************/

	/**************************/
	/* Recuperation du groupe */
	/**************************/
	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(grp_choix1));
	/* Parcours de la liste */
  while(pList)
  {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data)))
    {
        sLabel = gtk_button_get_label(GTK_BUTTON(pList->data));
        pList = NULL;
    }
    else
    {
        pList = g_slist_next(pList);
    }
  }
	
	if (!crv_strncmp("Groupe existant: ", sLabel)) {
		/* On recupere la categorie choisis dans la combobox*/
		int j = -1;
		j = gtk_combo_box_get_active(GTK_COMBO_BOX(grp_list_combobox));
		if (j != -1)
			grp = strdup(groupe[j]);
	} else
	if (!crv_strncmp("Choix nouveau groupe : ", sLabel)) {
		/* On recupere la nouvelle categorie dans la zone de texte */
		const gchar *sText;
		sText = gtk_entry_get_text(GTK_ENTRY(grp_entry));
		if (strlen(sText) == 0) {
			Warning ("\nGroupe manquant !");
			return;
		}
		grp = strdup(sText);
	}
	/****************************/
	/* Fin de la section GROUPE */
	/****************************/

	/***************************************/
	/* Section recuperation du commentaire */
	/***************************************/
	GtkTextBuffer* pTextBuffer;
	GtkTextIter iStart;
	GtkTextIter iEnd;
	gchar* sBuffer;
	pTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pTextView));
	/* On recupere l'origine du buffer */
	gtk_text_buffer_get_start_iter(pTextBuffer, &iStart);
	/* On recupere la fin du buffer */
	gtk_text_buffer_get_end_iter(pTextBuffer, &iEnd);

	/* On copie le contenu du buffer dans une variable */
	sBuffer = gtk_text_buffer_get_text(pTextBuffer, &iStart, &iEnd, TRUE);
	/***********************************/
	/* Fin recuperation du commentaire */
	/***********************************/

	/* Create info's file */
  (void)crv_strncpy (Description, sChemin, sizeof(Description));
  (void)crv_strncat (Description, "-DSCR-.xml",  sizeof (Description));
	fd = crv_fopen(Description, "w");
	if (fd == NULL )
	{
		Warning ("Creation du fichier de description impossible!");
		g_free(sChemin);
		g_free(sBuffer);
		crv_free (grp);
		crv_free (categorie);
		Close_widget(up_win);
		return;
	}

	sha1 = crv_sha1(sChemin);
	tmp = crv_strdup(sChemin);
	truename = basename((const char *)tmp);
	crv_free(tmp);
	if (truename == NULL)
	{
		fclose(fd);
		g_free(sChemin);
		crv_free(sha1);
		crv_free (grp);
		g_free(sBuffer);
		crv_free (categorie);
		Close_widget(up_win);
		return;
	}

	/* Ecriture du fichier d'info */
	fprintf(fd, "%s\n", "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>");
	fprintf(fd, "%s%s%s\n", "<file name=\"", sha1, "\">" );
	fprintf(fd, "%s%s%s\n", "<title><![CDATA[", truename,"]]></title>");
	fprintf(fd, "%s%s%s\n", "<who group=\"", grp,"\" name=\"\"/>");
	fprintf(fd, "%s%s%s\n", "<about>\n<genre>", categorie, "</genre>");
	if (strlen(sBuffer) > 0) {
		fprintf(fd, "%s\n", "<comment>\n<![CDATA[");
		fprintf(fd, "%s", sBuffer);
		fprintf(fd, "%s\n", "]]></comment>");
	}
	fprintf(fd, "%s\n", "</about>\n</file>");
	/* Fin de l'ecriture */

	printf("On balance le tout sur le serveur '%s'\n", server);
	g_free(sBuffer);
	crv_free (grp);
	fclose(fd);
	crv_free (categorie);
	pid_t pid;
	
	if ((pid = fork()) < 0)
	{
		printf("fork failed\n");
		return;
	}
	
	if (pid == 0)
	{
		/* child */
		printf("sha1: (avant Gk_Put) '%s'\n", sha1);
		Gtk_Put(sChemin, Description, server, sha1);
		g_free(sChemin);
		crv_free(sha1);
		crv_free(server);
		exit(EXIT_SUCCESS);
	}
	else
	{
		g_free(sChemin);
		crv_free(sha1);
		crv_free(server);
		Close_widget(up_win);
	return;
	}
}

/* Ouvre une fenetre de selection de fichier */
static void Select_file(GtkWidget *widget, gpointer data)
{
	GtkWidget *pFileSelection;
	GtkWidget *pParent;
	
	pParent = GTK_WIDGET(data);
	/* Creation de la fenetre de selection */
  pFileSelection = gtk_file_chooser_dialog_new("Ouvrir...",
		GTK_WINDOW(pParent),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		NULL);
	
	 //gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(pFileSelection), "~");

  
	/* On limite les actions a cette fenetre */
  gtk_window_set_modal(GTK_WINDOW(pFileSelection), TRUE);
	sChemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(pFileSelection));

  /* Affichage fenetre */
	
  switch(gtk_dialog_run(GTK_DIALOG(pFileSelection)))
  {
		case GTK_RESPONSE_OK:
			sChemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(pFileSelection));
			//printf("Chemin: %s\n", sChemin);
			gtk_label_set_label(path_label, sChemin);
			//g_free(sChemin);
			break;
		default:
			break;
  }

	gtk_widget_destroy(pFileSelection);
}

/* Fonction qui ouvre une fenetre permettant d'uploader un fichier */
static void Upload_file(GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	GtkWidget *pVBox;
	GtkWidget *HBox_fileselect;
	GtkWidget *file_select;
	GtkWidget *pSeparator;
	GtkWidget *pButton[2];
	GtkWidget *HBox;
	GtkWidget *pScrollbar;
	GtkWidget *pFrame;
	GtkWidget *VBox_cat;
	GtkWidget *HBox_cat1;
	GtkWidget *HBox_cat2;
	FILE *fd = NULL;

	int i;

	sChemin = NULL;
	
	/* Creation de la fenetre */
  up_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  /* Definition de la position */
	gtk_window_set_position(GTK_WINDOW(up_win), GTK_WIN_POS_CENTER);
	/* Definition de la taille de la fenetre */
  gtk_window_set_default_size(GTK_WINDOW(up_win), 0, 0);
	gtk_container_set_border_width(GTK_CONTAINER(up_win), 4);
  /* Titre de la fenetre */
  gtk_window_set_title(GTK_WINDOW(up_win), "Upload");

  /* Connexion du signal "destroy" */
  g_signal_connect(G_OBJECT(up_win), "destroy", G_CALLBACK(Close_widget), (GtkWidget*) up_win);
	
	/* Creation d'un VBox */
  pVBox = gtk_vbox_new(FALSE, 0);
	/* Ajout de la VBox a la fenetre principale */
	gtk_container_add(GTK_CONTAINER(up_win), pVBox);

	/*************************/
	/* Selection du fichier  */
	/*************************/
	pFrame = gtk_frame_new("Selection du fichier");
  gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);
	
	/*  le bouton de selection ainsi qu'un label qui contiendra le path du fichier */
  HBox_fileselect = gtk_hbox_new(FALSE, 0);
	
	/* Ajout de la Frame a la HBox_fileselect */
	gtk_container_add(GTK_CONTAINER(pFrame), HBox_fileselect);

	file_select=gtk_button_new_with_mnemonic("_Explorer...");
	
	/* Ajout de Bouton de selection dans la GtkVBox */
  gtk_box_pack_start(GTK_BOX(HBox_fileselect), file_select, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(file_select), "clicked", G_CALLBACK(Select_file), NULL);
	
	/* Creation du label */
  path_label = gtk_label_new("Selectionnez un fichier");
	/* On ajoute le label a l'interieur de la fenetre */
  gtk_container_add(GTK_CONTAINER(HBox_fileselect), path_label);

	/* Creation d un GtkHSeparator */
  pSeparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(pVBox), pSeparator, TRUE, FALSE, 0);
	/**************************/
	/* Fin selection fichier */
	/*************************/


	/*************************/
	/* Selection du serveur  */
	/*************************/
	pFrame = gtk_frame_new("Selection du serveur");
  gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);
	
	/*  le bouton de selection ainsi qu'un label qui contiendra le path du fichier */
  HBox_fileselect = gtk_hbox_new(FALSE, 0);
	
	/* Ajout de la Frame a la HBox_fileselect */
	gtk_container_add(GTK_CONTAINER(pFrame), HBox_fileselect);

	/* Creation de GtkComboBox */
  srv_list_combobox = gtk_combo_box_new_text();
  /* Ajout de GtkComboBox */
  gtk_box_pack_start(GTK_BOX(HBox_fileselect), srv_list_combobox, TRUE, TRUE, 0);
	
	char path[SIZE];
	char buf[SIZE];

	(void)crv_strncpy(path, options.path, sizeof(path));
	(void)crv_strncat(path, "serveur_liste", sizeof(path));

	fd = crv_fopen(path, "r");
	if (fd == NULL) {
		Warning("Lecture du fichier\nserveur_liste impossible");
		return;
	}
	i = 0;
	while(fgets(buf, sizeof(buf), fd) != NULL)
	{
		buf[strcspn(buf, "\n")] = '\0';
		gtk_combo_box_insert_text(GTK_COMBO_BOX(srv_list_combobox), i, buf);
		i++;
	}
	fclose(fd);
	
	/* "Activer" le premier element */
  gtk_combo_box_set_active(GTK_COMBO_BOX(srv_list_combobox), 0);

	/* Creation d un GtkHSeparator */
  pSeparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(pVBox), pSeparator, TRUE, FALSE, 0);
	/*************************/
	/* Fin selection serveur */
	/************************/


	
	/*************/
	/* CATEGORIE */
	/*************/

	/* Creation du premier GtkFrame */
  pFrame = gtk_frame_new("Categorie");
  gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);
	
	/* Boite contenant toute la partie categorie */
	VBox_cat = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), VBox_cat);
	
	/* Ligne dans la boite des categorie */
	/* Categorie existante               */
	HBox_cat1 = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(VBox_cat), HBox_cat1);

	/* Ligne dans la boite des categorie */
	/* Nouvelle Categorie                */
	HBox_cat2 = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(VBox_cat), HBox_cat2);

	/* Creation du premier bouton radio */
  cat_choix1 = gtk_radio_button_new_with_label(NULL, "Categorie existante: ");
  gtk_box_pack_start(GTK_BOX (HBox_cat1), cat_choix1, FALSE, FALSE, 0);

	/* Creation de GtkComboBox */
  cat_list_combobox = gtk_combo_box_new_text();
  /* Ajout de GtkComboBox */
  gtk_box_pack_start(GTK_BOX(HBox_cat1), cat_list_combobox, TRUE, TRUE, 0);

	
	for (i=0; genre[i]; i++)
	{
		if (!strstr(genre[i], "\n")) {
			gtk_combo_box_insert_text(GTK_COMBO_BOX(cat_list_combobox), i, genre[i]);
		}
	}
	/* "Activer" le premier element */
  gtk_combo_box_set_active(GTK_COMBO_BOX(cat_list_combobox), 0);

	cat_choix2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (cat_choix1), "Choix nouvelle categorie : ");
  gtk_box_pack_start(GTK_BOX (HBox_cat2), cat_choix2, FALSE, FALSE, 0);

	cat_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(HBox_cat2), cat_entry, TRUE, TRUE, 0);
	/* Fin de la categorie */

	/**********/
	/* GROUPE */
	/**********/
	/* Creation du premier GtkFrame */
  pFrame = gtk_frame_new("Groupe ");
  gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);
	
	/* Boite contenant toute la partie categorie */
	VBox_cat = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), VBox_cat);
	
	/* Ligne dans la boite des categorie */
	/* Categorie existante               */
	HBox_cat1 = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(VBox_cat), HBox_cat1);

	/* Ligne dans la boite des categorie */
	/* Nouvelle Categorie                */
	HBox_cat2 = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(VBox_cat), HBox_cat2);

	/* Creation du premier bouton radio */
  grp_choix1 = gtk_radio_button_new_with_label(NULL, "Groupe existant: ");
  gtk_box_pack_start(GTK_BOX (HBox_cat1), grp_choix1, FALSE, FALSE, 0);

	/* Creation de GtkComboBox */
  grp_list_combobox = gtk_combo_box_new_text();
  /* Ajout de GtkComboBox */
  gtk_box_pack_start(GTK_BOX(HBox_cat1), grp_list_combobox, TRUE, TRUE, 0);

	for (i=0; groupe[i]; i++)
	{
		if (!strstr(groupe[i], "\n")) {
			gtk_combo_box_insert_text(GTK_COMBO_BOX(grp_list_combobox), i, groupe[i]);
		}
	}
	/* "Activer" le premier element */
  gtk_combo_box_set_active(GTK_COMBO_BOX(grp_list_combobox), 0);

	grp_choix2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (grp_choix1), "Choix nouveau groupe : ");
  gtk_box_pack_start(GTK_BOX (HBox_cat2), grp_choix2, FALSE, FALSE, 0);

	grp_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(HBox_cat2), grp_entry, TRUE, TRUE, 0);
	/* Fin des groupes */
	
	/* Creation d un GtkHSeparator */
  pSeparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(pVBox), pSeparator, TRUE, TRUE, 0);

	/****************************************************/
	/* Creation de la zone de texte pour le commentaire */
	/****************************************************/

	/* Creation du premier GtkFrame */
  pFrame = gtk_frame_new("Commentaire ");
  gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, TRUE, 0);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(pFrame),pScrollbar);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	pTextView = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(pScrollbar), pTextView);
	/* Fin de la partie zone de texte */



	
	/* Creation d un GtkHSeparator */
  pSeparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(pVBox), pSeparator, TRUE, FALSE, 0);
	/**************************/
	/* Boutton de fin de page */
	/**************************/
	HBox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pVBox), HBox);

	/* Creation de deux bouttons */
	pButton[0] = gtk_button_new_with_label("Uploader");
	pButton[1] = gtk_button_new_with_label("Annuler");

	/* Ajout des boutons dans la boite horizontale */
	gtk_box_pack_start(GTK_BOX(HBox), pButton[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(HBox), pButton[1], FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(pButton[1]), "clicked", G_CALLBACK(gtk_widget_destroy), (GtkWidget*)up_win);
	g_signal_connect(G_OBJECT(pButton[0]), "clicked", G_CALLBACK(Valid_upload), NULL);

	/* Affichage de la fenetre */
	gtk_widget_show_all(up_win);
}




/* Fonction appellee quand on veux des informations sur un fichier */
/* La fonction ouvre un nouvel onglet et affiche les informations */
static void Gtk_Info(GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	if (choice_in_list == NULL)
		return;
	
	fprintf(stderr, "On cherche l'id n°%s\n", choice_in_list);
	
	xmlXPathContextPtr xml_context = NULL;
	xmlXPathObjectPtr xmlobject;
	GtkWidget *pScrollbar;
	GtkWidget *pTabLabel;
	GtkWidget *pVBox;
	GtkWidget *pButton[3];
	GtkWidget *pHBox;

	char *path = NULL;
	char xpath[SIZE];
	char buf[4096];
	char *ID = NULL;
	ID = crv_strdup(choice_in_list);


	pScrollbar = gtk_scrolled_window_new(NULL, NULL);

	/* Creation d'un VBox */
  pVBox = gtk_vbox_new(FALSE, 0);

	/* Creation de deux bouttons */
	pButton[0] = gtk_button_new_with_label("Telecharger");
	pButton[1] = gtk_button_new_with_label("Fermer l'onglet");
	
	/* creation d'une boite horizontale dans laqeulle on va mettre les bouttons */
	pHBox = gtk_hbox_new(TRUE, 0);
	
	/* creation d'une scroolbar pour la boite verticale VBox */
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), pVBox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), 
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pScrollbar), GTK_SHADOW_IN);

	(void)crv_strncpy(xpath, "/database/file[@id='", sizeof(xpath));
	(void)crv_strncat(xpath, choice_in_list, sizeof(xpath));
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
				/* creation de l'onglet qui porte le nom du fichier */
				pTabLabel = gtk_label_new(node->content);
				gtk_notebook_append_page(GTK_NOTEBOOK(pNotebook), pScrollbar, pTabLabel);

				/* Ajout du titre dans l'onglet */
				GtkWidget *label;
				label= gtk_label_new(NULL);
				snprintf(buf, sizeof(buf), 
					"Titre  : <span font_desc=\"Times New Roman italic 12\" foreground=\"#0000FF\">%s</span>", 
					node->content);
				gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
				gtk_label_set_markup(GTK_LABEL(label), buf);
				gtk_box_pack_start(GTK_BOX(pVBox), label, FALSE, FALSE, 0);

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
					(void)snprintf(mo, sizeof(mo), " %ld Go", lval/(1024*1024*1024));
				else if (lval > 1024*1024)
					(void)snprintf(mo, sizeof(mo), " %ld Mo", lval/(1024*1024));
				else
					(void)snprintf(mo, sizeof(mo), " %ld Ko", lval/(1024));

				/* Ajout de la taille dans l'onglet */
				GtkWidget *label;
				label= gtk_label_new(NULL);
				snprintf(buf, sizeof(buf), "Taille  : %s", mo);
				gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
				gtk_label_set_markup(GTK_LABEL(label), buf);
				gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
				gtk_box_pack_start(GTK_BOX(pVBox), label, FALSE, FALSE, 0);

			}
			/* Genre */
			if (!crv_strncmp(node->parent->name , "genre") ) {
				/* Ajout du titre dans l'onglet */
				GtkWidget *label;
				GtkWidget *frame;
				label= gtk_label_new(NULL);
				snprintf(buf, sizeof(buf), "Genre: %s\n", node->content);
				gtk_label_set_markup(GTK_LABEL(label), buf);
				gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
				(void)gtk_label_set_use_markup(label, TRUE);
				(void)gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
				gtk_box_pack_start(GTK_BOX(pVBox), label, FALSE, FALSE, 0);
			}
			/* Comment */
			if (!crv_strncmp(node->parent->name , "comment") ) {
				/* Ajout du titre dans l'onglet */
				GtkWidget *label;
				GtkWidget *pFrame;
				label= gtk_label_new(NULL);
				pFrame = gtk_frame_new("Commentaire");
				snprintf(buf, sizeof(buf), "%s", node->content);
				gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
				gtk_label_set_markup(GTK_LABEL(label), buf);
				(void)gtk_label_set_use_markup(label, TRUE);
				gtk_container_add(GTK_CONTAINER(pFrame), label);
				gtk_box_pack_start(GTK_BOX(pVBox), pFrame, FALSE, FALSE, 0);
			}

		}
	}
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);

	/* Ajout de la boite horizontale a la boite verticale*/
	gtk_box_pack_start(GTK_BOX(pVBox), pHBox, FALSE, FALSE, 0);

	/* Ajout des boutons dans la boite horizontale */
	gtk_box_pack_start(GTK_BOX(pHBox), pButton[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pHBox), pButton[1], FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(pButton[1]), "clicked", G_CALLBACK(close_tab), NULL);
	g_signal_connect(G_OBJECT(pButton[0]), "clicked", G_CALLBACK(Gtk_download), ID);
	
	gtk_widget_queue_draw (GTK_WIDGET (pNotebook));
	gtk_widget_show_all(pWindow);

}


/* Gtk_Diff */
static void Gtk_Diff(GtkWidget *pEntry, gpointer data)
{
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	GtkTreeIter pIter;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;

	gtk_tree_store_clear(pTreeStore);

	/* recuperation des id */
	int j = 0;
	char **ID = NULL;
	xmlXPathContextPtr xml_context_id;
	xmlXPathObjectPtr xmlobject_id;
	xmlXPathInit();

	ID = calloc(4096 * sizeof *ID, sizeof(char *));

	xml_context_id = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file[@new='1']");
	xmlobject_id = xmlXPathEval (path, xml_context_id);
	crv_free(path);
	path = NULL;
	if ((xmlobject_id->type == XPATH_NODESET) && (xmlobject_id->nodesetval))
  {
		int i;
		xmlNodePtr node;
		for (i = 0; i < xmlobject_id->nodesetval->nodeNr; i++)
		{
			node = xmlobject_id->nodesetval->nodeTab[i];
			xmlChar *xml_id = xmlGetProp(node, "id");
			ID[j] = crv_strdup((const char *)xml_id);
			xmlFree (xml_id);
			j++;
		}
	}
	
	xmlXPathFreeObject (xmlobject_id);
  xmlXPathFreeContext (xml_context_id);
	/* Fin de recuperation des id */

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file[@new='1']/*/text()");
	xmlobject = xmlXPathEval (path, xml_context);
	crv_free(path);
	
	j = 0;
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
					char *ep = NULL;
					char mo[SIZE];
					long lval;
					Size = crv_strdup(node->content);
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
					
					gtk_tree_store_append(pTreeStore, &pIter, NULL);
					gtk_tree_store_set(pTreeStore, &pIter, N, ID[j], P, Title, Q, mo, -1);
					j++;
					crv_free(Title);
					crv_free(Size);
				}
			}
		}
	}
	for ( j = 0; ID[j]; j++)
		crv_free(ID[j]);
	crv_free(ID);
	xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);

}


/* Gtk_Find */
static void Gtk_Find(GtkWidget *pEntry, gpointer data)
{
	const gchar *key;
	/* Recuperation du texte contenu dans le GtkEntry */
	key = gtk_entry_get_text(GTK_ENTRY(pEntry));
	printf("Keyword='%s'\n", key);
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr xmlobject;
	GtkTreeIter pIter;
	char *path = NULL;
	char *Title = NULL;
	char *Size = NULL;
	char Id_char[SIZE];
	int Id = 0;
	int flag = -1;

	gtk_tree_store_clear(pTreeStore);

	xmlXPathInit();
	xml_context = xmlXPathNewContext (xmldoc);
	path = crv_strdup("/database/file/*/text()");
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
				if (!crv_strncmp(node->parent->name , "title") ) {
					char *r = NULL;
					Id++;
					r = crv_strcasestr( node->content, key);
					if (r != NULL) {
						flag = 1;
						Title = crv_strdup(node->content);
					}
				}
				/* Size */
				if (!crv_strncmp(node->parent->name , "size") && (flag == 1)) {
					Size = crv_strdup(node->content);
					char *ep = NULL;
					char mo[SIZE];
					long lval;
					Size = crv_strdup(node->content);
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
					(void)snprintf(Id_char, sizeof(Id_char), "%d", Id);

					gtk_tree_store_append(pTreeStore, &pIter, NULL);
					gtk_tree_store_set(pTreeStore, &pIter, N, Id_char, P, Title, Q, mo, -1);
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
	/* Modification du texte contenu dans le GtkLabel */
  gtk_entry_set_text(GTK_ENTRY(pEntry), "");

}

/* Generate Gtk GUI */
void gen_Gtk_gui(int argc, char **argv)
{
	GtkWidget *pVBox;
	GtkWidget *Tool_HBox;
  GtkWidget *pToolbar = NULL;
	GtkWidget *pEntry;
	GtkWidget *pScrollbar;
	GtkWidget *load_scrollbar;
	GtkWidget *Select_Button;
	GtkTreeSelection *selection;

  GtkTreeViewColumn *pColumn = NULL;
  GtkCellRenderer *pCellRenderer;
	choice_in_list = NULL;

	gtk_init(&argc, &argv);
     
  /* Creation de la fenetre principale
	 * pWindows = principale Windows
	 */
  pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(pWindow), "gCreuvux v0.72 ");
  gtk_window_set_default_size(GTK_WINDOW(pWindow), 640, 480);
	gtk_container_set_border_width(GTK_CONTAINER(pWindow), 1);
  g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(gtk_main_quit), NULL);
   
  /* Creation de la pVBox */
  pVBox = gtk_vbox_new(FALSE, 0);
	/* Ajout de la pVBox dans la fenetre */
  gtk_container_add(GTK_CONTAINER(pWindow), pVBox);
		

	/* Creation de la barre d'outils */
  pToolbar = gtk_toolbar_new();
	/* Ajout de la barre d'outil dans pVBox */
	gtk_box_pack_start(GTK_BOX(pVBox), pToolbar, FALSE, FALSE, 0);

	// Creation du bouton SYNC
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_REFRESH,
      "Afficher liste",
      NULL,
      G_CALLBACK(sync_boutton),
      pColumn,
      -1);
	 
	// Creation du bouton INFO
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_PROPERTIES,
      "En savoir plus sur le fichier selectionne",
      NULL,
      G_CALLBACK(Gtk_Info),
      pColumn,
      -1);
	
	// Creation du bouton NEW
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_NEW,
      "Afficher les nouveautes",
      NULL,
      G_CALLBACK(Gtk_Diff),
      pColumn,
      -1);
	
	// Creation du bouton d'UPLOAD
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_OPEN,
      "Deposer un fichier",
      NULL,
      G_CALLBACK(Upload_file),
      pColumn,
      -1);
	
	
	// Insertion d'un espace	
	gtk_toolbar_append_space(GTK_TOOLBAR(pToolbar));

	// Creation du bouton pour la modification du fichier de config
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_PREFERENCES,
			"Deposer un fichier",
      NULL,
      G_CALLBACK(NULL),
      pColumn,
      -1);

   gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_QUIT,
      "Fermer",
      NULL,
      G_CALLBACK(gtk_main_quit),
      NULL,
      -1);
	 

   // Modification de la taille des icones
   gtk_toolbar_set_icon_size(GTK_TOOLBAR(pToolbar), GTK_ICON_SIZE_DIALOG);

   // Affichage uniquement des icones
   gtk_toolbar_set_style(GTK_TOOLBAR(pToolbar), GTK_TOOLBAR_BOTH);

	
	/*************************************/
	/* Outils de recherche dans la liste */
	/*************************************/

	/* Creation de la box horizontale  qui va contenir les 
	 * options de recherche et de selection de la liste
	 */
  Tool_HBox = gtk_hbox_new(FALSE, 0);
	/* Ajout de la Tool_HBox dans la pVBox */
	gtk_box_pack_start(GTK_BOX(pVBox), Tool_HBox, FALSE, FALSE, 0);

	/* Creation de ListBox */
  ListBox = gtk_combo_box_new_text();
  /* Ajout de ListBox */
  gtk_box_pack_start(GTK_BOX(Tool_HBox), ListBox, FALSE, FALSE, 0);

	/* Ajout des elements a la liste ListBox*/
	gen_combo_box_list();
	/* remplissage de la liste des groupes */
	gen_grp_list();
  
	/* "Activer" le premier element */
  gtk_combo_box_set_active(GTK_COMBO_BOX(ListBox), 0);

	/* Ajout d'un bouton pour vaider le choix de la categorie */
	
	/* Creation d'un bouton "ajouter" */
	Select_Button = gtk_button_new_with_label(" Valider ");
  /* Ajout du bouton dans pHBox */
  gtk_box_pack_start(GTK_BOX(Tool_HBox), Select_Button, FALSE, FALSE, 0);
	/* Connexion du signal "clicked" sur un des elements de la liste */
  g_signal_connect(G_OBJECT(Select_Button), "clicked", G_CALLBACK(select_cat), ListBox);


	/* ajout du label Recherche */
	GtkWidget* find_label;
	find_label = gtk_label_new("Rechercher: ");
	gtk_box_pack_start(GTK_BOX(Tool_HBox), find_label, FALSE, FALSE, 0);

	/* Preparation de la zone de saisie */
	pEntry = gtk_entry_new();
	/* Insertion du GtkEntry dans la GtkVBox */
  gtk_box_pack_start(GTK_BOX(Tool_HBox), pEntry, TRUE, TRUE, 0);
	/* Connexion du signal "activate" du GtkEntry */
  g_signal_connect(G_OBJECT(pEntry), "activate", G_CALLBACK(Gtk_Find), NULL);

  /************************/
	/* Creation de la liste */
	/************************/
	/* Creation de la box horizontale  qui va contenir la liste */
  List_HBox = gtk_hbox_new(TRUE, 0);
	/* Ajout de la List_HBox dans la pVBox */
	gtk_box_pack_start(GTK_BOX(pVBox), List_HBox, TRUE, TRUE, 0);
	

	/**********************/
	/* Rajout des onglets */
	/**********************/
	
	/* Creation du GtkNotebook */
  pNotebook = gtk_notebook_new();
  
	/* Position des onglets : en haut */
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pNotebook), GTK_POS_TOP);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(pNotebook), 1 );
	gtk_box_pack_start(GTK_BOX(List_HBox), pNotebook, TRUE, TRUE, 0);
  
	GtkWidget *pTabLabel;


	// Mise en place d'un arbre vide
  // Creation du modele
  pTreeStore = gtk_tree_store_new(N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  // Creation de la vue
  pTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pTreeStore));

	//Widget servant a la selection dans la liste
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pTreeView));

  // Creation de la premiere colonne
  pCellRenderer = gtk_cell_renderer_text_new();
  pColumn = gtk_tree_view_column_new_with_attributes("Id", pCellRenderer, "text", N, NULL);
  // Ajout de la colonne à la vue
  gtk_tree_view_append_column(GTK_TREE_VIEW(pTreeView), pColumn);

    
	// Creation de la deuxieme colonne
  pCellRenderer = gtk_cell_renderer_text_new();
  pColumn = gtk_tree_view_column_new_with_attributes("Titre", pCellRenderer, "text", P, NULL);
  // Ajout de la colonne à la vue
  gtk_tree_view_append_column(GTK_TREE_VIEW(pTreeView), pColumn);

		
	// Creation de la troisieme colonne
  pCellRenderer = gtk_cell_renderer_text_new();
  pColumn = gtk_tree_view_column_new_with_attributes("Taille", pCellRenderer, "text", Q, NULL);
  // Ajout de la colonne à la vue
  gtk_tree_view_append_column(GTK_TREE_VIEW(pTreeView), pColumn);
	
	// Ajout de la scrollbar verticale
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	/* Ligne qui fait chier */
	gtk_container_add(GTK_CONTAINER(pScrollbar), pTreeView);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), 
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pScrollbar), GTK_SHADOW_IN);

	/* Creation de la tabulation de la liste */
  pTabLabel = gtk_label_new("Liste Distante");
	gtk_notebook_append_page(GTK_NOTEBOOK(pNotebook), pScrollbar, pTabLabel);

	/****************************************/
	/* creation de l'onglet Download/Upload */
	/****************************************/
	// Mise en place d'un arbre vide
  // Creation du modele
  load_tree = gtk_tree_store_new(N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE);

  // Creation de la vue
  load_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(load_tree));

	//Widget servant a la selection dans la liste
	load_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(load_view));

  // Creation de la premiere colonne
  pCellRenderer = gtk_cell_renderer_text_new();
  pColumn = gtk_tree_view_column_new_with_attributes("Nom du fichier", pCellRenderer, "text", N, NULL);
  // Ajout de la colonne à la vue
  gtk_tree_view_append_column(GTK_TREE_VIEW(load_view), pColumn);

    
	// Creation de la deuxieme colonne
  pCellRenderer = gtk_cell_renderer_text_new();
  pColumn = gtk_tree_view_column_new_with_attributes("Rate", pCellRenderer, "text", P, NULL);
  // Ajout de la colonne à la vue
  gtk_tree_view_append_column(GTK_TREE_VIEW(load_view), pColumn);

		
	// Creation de la troisieme colonne
  pCellRenderer = gtk_cell_renderer_progress_new();
  pColumn = gtk_tree_view_column_new_with_attributes("Progression", pCellRenderer, "value", Q, NULL);
  // Ajout de la colonne à la vue
  gtk_tree_view_append_column(GTK_TREE_VIEW(load_view), pColumn);
	
	// Ajout de la scrollbar verticale
	load_scrollbar = gtk_scrolled_window_new(NULL, NULL);
	/* Ligne qui fait chier */
	gtk_container_add(GTK_CONTAINER(load_scrollbar), load_view);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(load_scrollbar), 
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(load_scrollbar), GTK_SHADOW_IN);

	/* Creation de la tabulation de la liste */
  pTabLabel = gtk_label_new("Download/Upload");
	gtk_notebook_append_page(GTK_NOTEBOOK(pNotebook), load_scrollbar, pTabLabel);

	/****************/
	/* Barre d'etat */
	/****************/
	/* Creation de la barre d'etat */
	pStatusBar = gtk_statusbar_new();
	
	gtk_box_pack_end(GTK_BOX(pVBox), pStatusBar, FALSE, FALSE, 0);
	
	//Quand on clik sur une ligne de la liste on affiche le titre dans la barre d'etat.
	g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(get_selected), pStatusBar);


  gtk_widget_show_all(pWindow);

	/* Nettoyage de l'onglet du listing */
	gtk_tree_store_clear(pTreeStore);
	
	/* Chargement du fichier listing.xml */
	download_dom_in();
	List();
	g_timeout_add_seconds ((guint)1, (GSourceFunc)Print_load_progress, NULL);
	//g_timeout_add_seconds ((guint)1, (GSourceFunc)Action, NULL);
	//g_timeout_add ((guint)100, (GSourceFunc)Print_load_progress, NULL);
  
	gtk_main();
	
	/* Libere l'espace memmoire du fichier listing.xml */
	download_dom_out();

}
