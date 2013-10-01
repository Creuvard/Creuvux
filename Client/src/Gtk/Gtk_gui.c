/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 * This program is the creuvux client.  It connect to servers,
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

#include <libgen.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <pthread.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/*
 * Glib/Gtk+-2.0
 */
#include <gtk/gtk.h>
#include <glib/gprintf.h>

/* SQLite STUFF */
#include "sqlite3.h"

#include "creuvard.h"
#include "client_conf.h"
#include "Gtk_gui.h"
#include "sync.h"
#include "help.h"
#include "msg.h"

/*
#include "help.h"
#include "sync.h"
#include "msg.h"
#include "catls.h"
#include "thread.h"
#include "Gtk_get.h"
#include "Gtk_gui.h"
#include "Gtk_put.h"
*/


#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Client configuration options. */
ClientOptions options;

static sqlite3 *db;
static char **categories; /* Utiliser pour la combobox */
static char **groupe;     /* Utiliser l'ors de l'envoie d'un fichier */
static int num_categorie;
static int num_groupe;

/*
 * Element a afficher
 */
char *cat;
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


static void Send_msg(GtkWidget *widget, GtkTreeViewColumn *pColumn);


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

GtkTreeStore *load_tree;
static GtkTreeSelection *load_selection;
static GtkWidget		*load_view;

static void current_work(const char *msg)
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

	gtk_tree_store_clear(GTK_TREE_STORE(load_tree));
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
					if (pct <= 100) {
						(void)crv_strncpy(Bp, a[2], sizeof(Bp));
						(void)crv_strncat(Bp, " Ko/s", sizeof(Bp));
						//fprintf(stderr," '%s' '%s' '%f'\n", a[0], Bp, pct);
						gtk_tree_store_append(GTK_TREE_STORE(load_tree), &pIter, NULL);
						gtk_tree_store_set(GTK_TREE_STORE(load_tree), &pIter, N, a[0], P, Bp, Q, (gdouble)pct, -1);
					}
				}
				memset(Bp, 0, sizeof(Bp));
				for ( i = 0; a[i]; i++)
					if (a[i] != NULL)
						crv_free(a[i]);
				if (a != NULL)
					crv_free(a);
			}
			fclose(fd);
		}
		}
		(void)closedir(dirp);
		fprintf(stderr, " ");
		return;
}

static void close_tab (GtkWidget *pWidget, gpointer filename)
{
	gint page;
	page = gtk_notebook_get_current_page (GTK_NOTEBOOK(pNotebook));
	gtk_notebook_remove_page (GTK_NOTEBOOK(pNotebook), page);
	//gtk_notebook_insert_page(GTK_NOTEBOOK(pNotebook), pLabel1, download_tab, 1);
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


static int Cat_callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
		categories[num_categorie] = crv_strdup(argv[i]);
	}
	num_categorie += 1;
  return 0;
}

/* Genere la liste deroulante */
static void gen_combo_box_list(void)
{
    char *zErrMsg = NULL;
	int rc = -1;
	int i;
	num_categorie = 0;
	gchar *tmp = NULL;
	categories = calloc(4096 * sizeof *categories, sizeof(char *));

	rc = sqlite3_exec(db, "SELECT DISTINCT Cat FROM Categories;", Cat_callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
				//crv_free(categories);
    }

	gtk_combo_box_insert_text(GTK_COMBO_BOX(ListBox), 0, "All");
	for (i=0; categories[i]; i++)
	{
		if (!strstr(categories[i], "\n")) {
			tmp = g_locale_to_utf8( categories[i], -1, NULL, NULL, NULL);
			gtk_combo_box_insert_text(GTK_COMBO_BOX(ListBox), i+1, tmp);
			g_free(tmp);
			tmp = NULL;
		}
	}

}

static int Grp_callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
		groupe[num_groupe] = crv_strdup(argv[i]);
	}
	num_groupe += 1;
  return 0;
}

/* Récupère la liste de tous les groupes */
static void gen_group_list(void)
{
    char *zErrMsg = NULL;
	int rc = -1;
	int i;
	num_groupe = 0;
	groupe = calloc(4096 * sizeof *groupe, sizeof(char *));

	rc = sqlite3_exec(db, "SELECT DISTINCT groupname FROM Grp_sha1;", Grp_callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
		crv_free(groupe);
    }
}

static int List_callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  int diff;
  char mo[SIZE];
  char tmp[SIZE];
  gchar *Name = NULL;
  char *Size = NULL;
  char *rowid = NULL;
  GtkTreeIter pIter;


  for(i=0; i<argc; i++){
      if (!crv_strncmp(azColName[i], "Name"))
        Name = g_locale_to_utf8(argv[i], -1, NULL, NULL, NULL);
			//= crv_strdup(argv[i]);
      else
      if (!crv_strncmp(azColName[i], "Size"))
		{
			char *ep;
			long lval;
			int j;

			lval = strtol(argv[i], &ep, 10);
			if (argv[i][0] == '\0' || *ep != '\0') {
				fprintf(stderr, "%s%s", argv[i], " is not a number");
				return;
			}

			if (lval > 1024*1024*1024)
				(void)snprintf(mo, sizeof(mo), "%ld Go", lval/(1024*1024*1024));
			else if (lval > 1024*1024)
				(void)snprintf(mo, sizeof(mo), "%ld Mo", lval/(1024*1024));
			else
				(void)snprintf(mo, sizeof(mo), "%ld Ko", lval/(1024));
			Size = crv_strdup(mo);
		} else
		if (!crv_strncmp(azColName[i], "rowid"))
            rowid = crv_strdup(argv[i]);

  }

  //(void)snprintf(tmp, sizeof(tmp), "%d", id);
  //rowid = crv_strdup(tmp);
  gtk_tree_store_append(pTreeStore, &pIter, NULL);
  gtk_tree_store_set(pTreeStore, &pIter, N, rowid, P, Name, Q, Size, -1);

  //memset(tmp, 0, sizeof(tmp));
  memset(mo, 0, sizeof(mo));

  if (Name != NULL)
    g_free(Name);
  if (rowid != NULL)
    crv_free(rowid);
  if (Size != NULL)
    crv_free(Size);
  return 0;
}

static void List(void)
{
    char *zErrMsg = 0;
	int rc = -1;
	rc = sqlite3_exec(db, "SELECT DISTINCT rowid,Name,Size FROM Files;" , List_callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

static void select_cat(GtkWidget *widget, gpointer data)
{
	GtkTreeIter pIter;
	int j;
	char *zErrMsg = 0;
	char *req = NULL;
	int rc = -1;

    j = gtk_combo_box_get_active(GTK_COMBO_BOX(data));
    if (j != 0)
        if (categories[j-1] == NULL)
            return;

    gtk_tree_store_clear(pTreeStore);
    if (j == 0)
        req = sqlite3_mprintf("SELECT DISTINCT Files.rowid,Name,Size FROM Files;");
    else
        req = sqlite3_mprintf("SELECT DISTINCT Files.rowid,Name,Size FROM Files,Categories WHERE Files.Sha1=Categories.Sha1 AND Categories.Cat='%q';", categories[j-1]);
	rc = sqlite3_exec(db, req , List_callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_free(req);
}

static void Find (GtkWidget *pEntry, gpointer data)
{
    GtkTreeIter pIter;
	int j;
	char *zErrMsg = 0;
	char *req = NULL;
	int rc = -1;
	const gchar *key;
	//gchar *tmp = NULL;

	/* Recuperation du texte contenu dans le GtkEntry */
	key = gtk_entry_get_text(GTK_ENTRY(pEntry));
	//gtk_statusbar_push(GTK_STATUSBAR(pStatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(pStatusBar), key), key);
	//tmp = g_locale_to_utf8( key, -1, NULL, NULL, NULL);
	req = sqlite3_mprintf("SELECT DISTINCT Files.rowid,Name,Size FROM Files WHERE Files.Name like '%q%q%q';", "%", gtk_entry_get_text(GTK_ENTRY(pEntry)), "%");
	//tmp = g_locale_to_utf8( req, -1, NULL, NULL, NULL);
	//printf("SQL>%s\n", req);
	gtk_tree_store_clear(pTreeStore);
	rc = sqlite3_exec(db, req, List_callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
  }
	//g_free(tmp);
  sqlite3_free(req);
}


static int Info_cb(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    info_title = NULL;
    for(i=0; i<argc; i++){
      if (!crv_strncmp(azColName[i], "Name"))
        info_title = crv_strdup(argv[i]);
    }
    return 0;
}

static int Info_cb_2(void *Box, int argc, char **argv, char **azColName){
    int i;
    GtkWidget *pBox;
    char buf[4096];
    gchar *buff;

    pBox = Box;
    info_title = NULL;
    for(i=0; i<argc; i++){
      //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");

      if (!crv_strncmp(azColName[i], "Cat"))
      {
            GtkWidget *label;
            label= gtk_label_new(NULL);
            snprintf(buf, sizeof(buf), "Categorie  : <span foreground=\"#0000FF\">%s</span>", argv[i]);
            buff = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
            gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
            gtk_label_set_markup(GTK_LABEL(label), buf);
            gtk_box_pack_start(GTK_BOX(pBox), label, FALSE, FALSE, 0);
            g_free(buff);
      }
      if (!crv_strncmp(azColName[i], "Size"))
      {
            GtkWidget *label;
            label= gtk_label_new(NULL);
            char mo[SIZE];
            char *ep;
			long lval;
			int j;

			lval = strtol(argv[i], &ep, 10);
			if (argv[i][0] == '\0' || *ep != '\0') {
				fprintf(stderr, "%s%s", argv[i], " is not a number");
				return;
			}

			if (lval > 1024*1024*1024)
				(void)snprintf(mo, sizeof(mo), "%ld Go", lval/(1024*1024*1024));
			else if (lval > 1024*1024)
				(void)snprintf(mo, sizeof(mo), "%ld Mo", lval/(1024*1024));
			else
				(void)snprintf(mo, sizeof(mo), "%ld Ko", lval/(1024));
            snprintf(buf, sizeof(buf), "Taille  : <span foreground=\"#0000FF\">%s</span>", mo);
            buff = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
            gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
            gtk_label_set_markup(GTK_LABEL(label), buf);
            gtk_box_pack_start(GTK_BOX(pBox), label, FALSE, FALSE, 0);
            g_free(buff);
      }
      if (!crv_strncmp(azColName[i], "groupname"))
      {
            GtkWidget *label;
            label= gtk_label_new(NULL);
            snprintf(buf, sizeof(buf), "Groupe  : <span foreground=\"#0000FF\">%s</span>", argv[i]);
            buff = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
            gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
            gtk_label_set_markup(GTK_LABEL(label), buf);
            gtk_box_pack_start(GTK_BOX(pBox), label, FALSE, FALSE, 0);
            g_free(buff);
      }
      if (!crv_strncmp(azColName[i], "Descr") && (strlen(argv[i]) > 1))
      {
            GtkWidget *label;
            label= gtk_label_new(NULL);
            //snprintf(buf, sizeof(buf), "Description  : \n<span font_desc=\"Times New Roman italic 10\" foreground=\"#0000FF\">%s</span>", argv[i]);
            snprintf(buf, sizeof(buf), "Description : \n<span foreground=\"#0000FF\">%s</span>", argv[i]);
            buff = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
            gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
            gtk_label_set_markup(GTK_LABEL(label), buff);
            gtk_box_pack_start(GTK_BOX(pBox), label, FALSE, FALSE, 0);
            g_free(buff);
      }
    }
    return 0;
}

static int Info_cb_3(void *Box, int argc, char **argv, char **azColName){
    int i;
    GtkWidget *pBox;
    char buf[4096];
    gchar *buff;
		char *msg = NULL;
		char *date = NULL;
		char *pseudo = NULL;

    pBox = Box;
		info_title = NULL;
    for(i=0; i<argc; i++){
      //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
			if (!crv_strncmp(azColName[i], "Descr") && (strlen(argv[i]) > 1))
				msg = crv_strdup(argv[i]);
			if (!crv_strncmp(azColName[i], "Pseudo"))
				pseudo = crv_strdup(argv[i]);
			if (!crv_strncmp(azColName[i], "Date"))
				date = crv_strdup(argv[i]);
    }

		GtkWidget *label;
		label= gtk_label_new(NULL);
		snprintf(buf, sizeof(buf), "Description (par <span foreground=\"#0000FF\">%s</span> le <span foreground=\"#0000FF\">%s</span>): \n<span foreground=\"#0000FF\">%s</span>", pseudo, date, msg);
		buff = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
		gtk_label_set_markup(GTK_LABEL(label), buff);
		gtk_box_pack_start(GTK_BOX(pBox), label, FALSE, FALSE, 0);
		g_free(buff);
		
		
		
		if(msg != NULL)
			crv_free(msg);
		if(pseudo != NULL)
			crv_free(pseudo);
		if(date != NULL)
			crv_free(date);

    return 0;
}

static void * pre_Gtk_Get(void *userdata)
{
	char *id = userdata;
	fprintf(stdout, "On veut l'id -> '%s'\n", id);
	Get(db, id);
	pthread_exit (NULL);
}

static void Gtk_download(GtkWidget *pWidget, gpointer id)
{
	int  pid;
	int socket_id;
	pthread_attr_t *thread_attributes;
  pthread_t tid;
	char *ID = NULL;
	ID = crv_strdup(id);
	fprintf(stderr, "gpointer = %s\n", (char *)ID);
	/*
	// Initialisation des attributs d'un futur thread pour qu'il soit détaché 
      if (NULL == (thread_attributes = malloc (sizeof *thread_attributes)))
        {
          fprintf (stderr, "Problème avec malloc()\n");
          exit (EXIT_FAILURE);
        }
      if (0 != pthread_attr_init (thread_attributes))
        {
          fprintf (stderr, "Problème avec pthread_attr_init()\n");
          free (thread_attributes);
          exit (EXIT_FAILURE);
        }
      if (0 !=
          pthread_attr_setdetachstate (thread_attributes,
                                       PTHREAD_CREATE_DETACHED))
        {
          fprintf (stderr, "Problème avec pthread_attr_setdetachstate()\n");
          free (thread_attributes);
          exit (EXIT_FAILURE);
        }
		// Création du thread
      if (0 !=
            pthread_create (&tid, thread_attributes, pre_Gtk_Get, ID))
        {
          fprintf (stderr, "Problème avec pthread_create()\n");
          free (thread_attributes);
          exit (EXIT_FAILURE);
        }
	// Libération des ressources nécessaires pour les attributs du thread 
      pthread_attr_destroy (thread_attributes);
	*/

	
	if ((pid = fork()) < 0)
	{
		printf("fork failed\n");
		return;
	}
	
	if (pid == 0)
	{
		//g_timeout_add_seconds ((guint)1, (GSourceFunc)print_load_progress, NULL);
		Get(db, ID);
		if(id != NULL)
			crv_free(ID);
		exit(EXIT_SUCCESS);
	}
	else
	{
	}
	
	
	return;
}

/* Fonction appellee quand on veux des informations sur un fichier */
/* La fonction ouvre un nouvel onglet et affiche les informations */
static void Info(GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	if (choice_in_list == NULL)
		return;
	//fprintf(stderr, "On cherche l'id n°%s\n", choice_in_list);

	GtkWidget *pScrollbar;
	GtkWidget *pTabLabel;
	GtkWidget *pVBox;
	GtkWidget *pButton[3];
	GtkWidget *pHBox;

	char *path = NULL;
	char xpath[SIZE];
	char buf[4096];
	char *zErrMsg = 0;
	int rc = -1;
	char *req = NULL;
	gchar *buff;
	char *ID = NULL;

	ID = crv_strdup(choice_in_list);
	req = sqlite3_mprintf("SELECT Name FROM Files WHERE rowid='%q'", ID);
	rc = sqlite3_exec(db, req , Info_cb, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_free(req);



	pScrollbar = gtk_scrolled_window_new(NULL, NULL);

	/* Creation d'un VBox */
   pVBox = gtk_vbox_new(FALSE, 0);

	/* Creation de deux bouttons */
	buff = g_locale_to_utf8("Télécharger", -1, NULL, NULL, NULL);
	pButton[0] = gtk_button_new_with_label(buff);
	g_free(buff);
	buff = g_locale_to_utf8("Fermer l'onglet", -1, NULL, NULL, NULL);
	pButton[1] = gtk_button_new_with_label(buff);
	g_free(buff);
	buff = g_locale_to_utf8("Commenter", -1, NULL, NULL, NULL);
	pButton[2] = gtk_button_new_with_label(buff);
	g_free(buff);


	/* creation d'une boite horizontale dans laqeulle on va mettre les bouttons */
	pHBox = gtk_hbox_new(TRUE, 0);

	/* creation d'une scroolbar pour la boite verticale VBox */
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), pVBox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pScrollbar), GTK_SHADOW_IN);

    /* AJout du titre de longlet */
		buff = g_locale_to_utf8(info_title, -1, NULL, NULL, NULL);
	pTabLabel = gtk_label_new(buff);
    gtk_notebook_append_page(GTK_NOTEBOOK(pNotebook), pScrollbar, pTabLabel);
		g_free(buff);

    /*************************/
	GtkWidget *label;
	label= gtk_label_new(NULL);
	snprintf(buf, sizeof(buf), "Titre  : <span font_desc=\"Times New Roman italic 12\" foreground=\"#0000FF\">%s</span>", info_title);
	buff = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_markup(GTK_LABEL(label), buff);
	gtk_box_pack_start(GTK_BOX(pVBox), label, FALSE, FALSE, 0);
	g_free(buff);

    /* Size */
    req = sqlite3_mprintf("SELECT Size FROM Files WHERE Files.rowid='%q'", ID);
	rc = sqlite3_exec(db, req , Info_cb_2, pVBox, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_free(req);

    /* Cat */
    req = sqlite3_mprintf("SELECT Cat FROM Categories WHERE Sha1 IN (SELECT Sha1 FROM Files WHERE rowid='%q' )", ID);
	rc = sqlite3_exec(db, req , Info_cb_2, pVBox, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_free(req);


    /* Grp */
	req = sqlite3_mprintf("SELECT groupname FROM Grp_Sha1 WHERE Sha1 IN (SELECT Sha1 FROM Files WHERE rowid='%q' )", ID);
	rc = sqlite3_exec(db, req , Info_cb_2, pVBox, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_free(req);

    /* Descr */
	req = sqlite3_mprintf("SELECT Descr,Pseudo,Date FROM Descr WHERE Sha1 IN (SELECT Sha1 FROM Files WHERE rowid='%q' ) ORDER BY Date ASC", ID);
	rc = sqlite3_exec(db, req , Info_cb_3, pVBox, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_free(req);

    /************************/
    /* Ajout de la boite horizontale a la boite verticale*/
	gtk_box_pack_start(GTK_BOX(pVBox), pHBox, FALSE, FALSE, 0);

	/* Ajout des boutons dans la boite horizontale */
	gtk_box_pack_start(GTK_BOX(pHBox), pButton[0], TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pHBox), pButton[2], TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pHBox), pButton[1], TRUE, TRUE, 0);


	g_signal_connect(G_OBJECT(pButton[1]), "clicked", G_CALLBACK(close_tab), NULL);
	g_signal_connect(G_OBJECT(pButton[0]), "clicked", G_CALLBACK(Gtk_download), ID);
    //g_signal_connect(G_OBJECT(pButton[0]), "clicked", G_CALLBACK(NULL), choice_in_list);
	g_signal_connect(G_OBJECT(pButton[2]), "clicked", G_CALLBACK(Send_msg), ID);

	gtk_widget_queue_draw (GTK_WIDGET (pNotebook));
	gtk_widget_show_all(pWindow);
	//crv_free(ID);

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
static GtkWidget *msg_View;
static GtkWidget *up_win;
static GtkWidget *msg_win;

/* Ferme le widget passe en parametre */
static void Close_widget(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
}

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
			gtk_label_set_label(GTK_LABEL(path_label), sChemin);
			//g_free(sChemin);
			break;
		default:
			break;
  }

	gtk_widget_destroy(pFileSelection);
}

static int create_db (sqlite3 *db)
{
	char *zErrMsg = 0;
	int rc = -1;
	/*
	 * CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL);
	 * CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL);
	 * CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL);
	 * CREATE TABLE Grp_Sha1 (groupname TEXT NOT NULL, sha1 TEXT NOT NULL);
	 * CREATE TABLE Grp_User (groupname TEXT NOT NULL, username TEXT NOT NULL);
	 */
	rc = sqlite3_exec(db, "CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Grp_Sha1 (groupname TEXT NOT NULL, sha1 TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_exec(db, "CREATE TABLE Grp_User (groupname TEXT NOT NULL, username TEXT NOT NULL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	return (0);
}

static int Put(sqlite3 *db, const char *filename, const char *server, const char *grp, const char *username, const char *genre, gchar *sComment)
{
  SSL_CTX *ctx = NULL;
  SSL*     ssl = NULL;
  int sd = -1;
  int port = -1;
  char *ep;
  long lval;
  int code = -1;
  off_t size = 0;
  char command[SIZE];
  char buf[SIZE];
  char size_c [SIZE];
  char *dscr = NULL;
	char *sha1 = NULL;
	char **a = NULL;
	char dbname[SIZE];
	char msg[4096];
	int descr_fd = 0;
	FILE *fd = NULL;
	char *fullpath = NULL;
  errno = 0;

  /* ETA */
  struct timeval tv1, tv2;
  int sec1;
	fullpath = crv_strdup(filename);
 
	/* On ouvre le fichier pour voir si il existe */
  if ((fd = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Le fichier '%s' peut pas etre ouvert\n", filename);
      fprintf (stderr, "Verifiez les droits et son emplacements\n");
      exit (EXIT_FAILURE);
    }
	fclose (fd);
	
	char *real_name = NULL;
	real_name = basename((const char *)filename);
	if (real_name == NULL)
		return (NULL);
  
  size = crv_du (filename);
  if (size == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  snprintf(size_c, sizeof(size), "%lld", size);

	fprintf(stdout, "%s\n", "Determination de la signature numerique du fichier en cours ...");
	sha1 = crv_sha1(filename);

  /* Create info's file */
  (void)crv_strncpy (dbname, sha1, sizeof(dbname));
  (void)crv_strncat (dbname, ".db",  sizeof (dbname));

	size = crv_du (filename);
  if (size == -1) {
		fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
		return(-1);
  }
  memset(size_c, 0, sizeof(size_c));
  snprintf(size_c, sizeof(size_c), "%lld", size);
	
	/*
	fprintf (stdout, "DEBUG: DB Name='%s'\n", dbname);
	fprintf (stdout, "DEBUG: Fileame='%s'\n",filename);
	fprintf (stdout, "DEBUG: Sha1='%s'\n", sha1); 
  fprintf (stdout, "DEBUG: Titre='%s'\n", real_name);
	fprintf (stdout, "DEBUG: Size='%ld'\n", size);
	fprintf (stdout, "DEBUG: Size='%s'\n", size_c);
	fprintf (stdout, "DEBUG: Pseudo='%s'\n", username);
	fprintf (stdout, "DEBUG: Message=\n`-> %s\n", sComment);
	*/
	/*
	 * Database for file creation
	 */
	char *zErrMsg = 0;
	int rc = -1;
	char *req = NULL;
	sqlite3 *db_put;
	/* Open MAIN Database */
	rc = sqlite3_open( dbname, &db_put);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db_put));
		sqlite3_close(db_put);
		return (-1);
	}

	/* Create Tables */
	if (create_db (db_put) == -1) {
		fprintf(stderr, "%s\n", "Put(): Database creation Failed");
		return (-1);
	}


	/* Fill Files tables*/
	/* CREATE TABLE Files (Sha1 TEXT NOT NULL, Name TEXT NOT NULL, Size NUMERIC NOT NULL); */
	req = sqlite3_mprintf("INSERT into Files (Sha1, Name, Size) values ('%q', '%q', '%q')", sha1, real_name, size_c);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	/* CREATE TABLE Categories (Sha1 TEXT NOT NULL, Cat TEXT NOT NULL); */
	req = sqlite3_mprintf("INSERT into Categories (Sha1, Cat) values ('%q', '%q')", sha1, genre);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	/* CREATE TABLE Descr (Sha1 TEXT NOT NULL, Descr LONGTEXT NOT NULL); */
	if (strlen(sComment) > 0) {
		req = sqlite3_mprintf("INSERT into Descr (Sha1, Descr) values ('%q', '%q')", sha1, sComment);
	} else
		req = sqlite3_mprintf("INSERT into Descr (Sha1, Descr) values ('%q', '')", sha1);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	/* CREATE TABLE Grp_Sha1 (groupname TEXT NOT NULL, sha1 TEXT NOT NULL); */
	req = sqlite3_mprintf("INSERT into Grp_Sha1 (groupname, sha1) values ('%q', '%q')", grp, sha1);
	rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_free(req);

	/* CREATE TABLE Grp_User (groupname TEXT NOT NULL, username TEXT NOT NULL); */
	if (username != NULL) {
		req = sqlite3_mprintf("INSERT into Grp_User (groupname, username) values ('%q', '%q')", grp, username);
		rc = sqlite3_exec(db_put, req, NULL, 0, &zErrMsg);
		if( rc!=SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		sqlite3_free(req);
	}
	sqlite3_close(db_put);

	/* Network zone */
	init_OpenSSL ();			  
  seed_prng ();
	a = crv_cut(server, ":");
  
	lval = strtol(a[1], &ep, 10);
  if (a[1][0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s\n", "Put(): Err[001] port is not a number");
		return (-1);
  }

  if ((errno == ERANGE && (lval == LONG_MAX
	  || lval == LONG_MIN)) ||
	  (lval > 65535 || lval < 0))
  {
		fprintf(stderr, "%s\n", "Put(): Err[002] port is out of range");
		return (-1);
  }
  port = lval;
  sd = crv_client_create( port, a[0], options.address_family);
  
  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "Put() Err[003] Create new ssl failed");
		return(-1);
  }

  /* connect the SSL object with a file descriptor */
  code = SSL_set_fd (ssl, sd);
  if ( code == 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "Put() Err[004] Put SSL on socket failed \n");
		return(-1);
  }

  code = SSL_connect (ssl);
  if (code == 0)	{
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[005] SSL_connect() failed");
		return(-1);
  } else
  if (code == -1) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf( stderr, "%s\n", "Put(): Err[006] SSL_connect() failed");
		return(-1);
  }

  /* Build command -> GET#version#sha1#begin#end */
  (void)crv_strncpy(command, "PUT#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, "0", sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
  
  /* Time initialisation */
  gettimeofday (&tv1, NULL);

  code = SSL_write (ssl, command, (int)strlen(command));
  if ( code <= 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_get_file(): Err[011] SSL_write() failed.");
	return(-1);
  }

  code = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[code] = '\0';

  if(!crv_strncmp (buf, "PUT_ACK"))
  {
		fprintf(stdout, "\n\n");
		code = SSL_sendfile(ssl, sha1, fullpath, (off_t)0, size);	
		if (code == -1) {
			close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
			fprintf(stderr, "%s\n", "Put() Err[012] SSL_sendfile() failed");
			return(-1);
		}
  }
	crv_free(fullpath);
  
  code = SSL_write (ssl, "PUT_END", (int)strlen("PUT_END"));
  if ( code <= 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[013] SSL_write() failed.");
		return(-1);
  }


  gettimeofday (&tv2, NULL);
  /* compute difference */
  sec1 = tv2.tv_sec - tv1.tv_sec;
  int min;
  float average;
  average = ((float) size / (float) sec1) / 1024;
  min = sec1 / 60;
  sec1 = sec1 - min * 60;
  fprintf (stdout,
	     "\n\nFile download in %d min  %d sec \nSpeed average -> %.f KBps\n\n",
	     min, sec1, average);

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);crv_free(sha1);


  /* Send file's decription */
  init_OpenSSL ();			  
  seed_prng ();
  sd = crv_client_create( port, a[0], options.address_family);
  
  /* We keep the certificate and key with the context. */
  ctx = setup_client_ctx ();

  /* TCP connection is ready. Do server side SSL. */
  ssl = SSL_new (ctx);
  if ((ssl)==NULL) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "Put() Err[003] Create new ssl failed");
	return(-1);
  }

  /* connect the SSL object with a file descriptor */
  code = SSL_set_fd (ssl, sd);
  if ( code == 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "Put() Err[004] Put SSL on socket failed \n");
	return(-1);
  }

  code = SSL_connect (ssl);
  if (code == 0)	{
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[005] SSL_connect() failed");
	return(-1);
  } else
  if (code == -1) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[006] SSL_connect() failed");
	return(-1);
  }

	size = crv_du (dbname);
  if (size == -1) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf( stderr, "%s\n", "Put(): Err[007] crv_du() can't get size");
	return(-1);
  }
  memset(size_c, 0, sizeof(size_c));
  snprintf(size_c, sizeof(size), "%lld", size);

	sha1 = crv_sha1(dbname);
  
  /* Build command -> GET#version#sha1#begin#end */
  (void)crv_strncpy(command, "COMMENT#", sizeof(command));
  (void)crv_strncat(command, CREUVUX_VERSION, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, sha1, sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, "0", sizeof(command));
  (void)crv_strncat(command, "#", sizeof(command));
  (void)crv_strncat(command, size_c, sizeof(command));
  
  code = SSL_write (ssl, command, (int)strlen(command));
  if ( code <= 0) {
	close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
	fprintf(stderr, "%s\n", "receive_get_file(): Err[011] SSL_write() failed.");
	return(-1);
  }

  code = SSL_read (ssl, buf, sizeof(buf) - 1);
  buf[code] = '\0';

  if(!strncmp (buf, "PUT_ACK", strlen("PUT_ACK")))
  {
		code = SSL_sendfile(ssl, sha1, dbname, (off_t)0, size);	
		if (code == -1) {
			close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
			fprintf(stderr, "%s\n", "Put() Err[012] SSL_sendfile() failed");
			return(-1);
		}
  }
  
  code = SSL_write (ssl, "PUT_END", (int)strlen("PUT_END"));
  if ( code <= 0) {
		close(sd); SSL_free (ssl); SSL_CTX_free (ctx);
		fprintf(stderr, "%s\n", "receive_get_file(): Err[013] SSL_write() failed.");
		return(-1);
  }


	fprintf(stdout, "%s", "Information about file is uploaded\n"); 

	code = unlink((const char *)dbname);
  if (code == -1) {
		fprintf(stderr, "%s%s\n", 
			"Put(): Err[010] unlink() failed with error -> ",
			strerror(errno));
		return (-1);
  }

  close(sd); SSL_free (ssl); SSL_CTX_free (ctx);crv_free(sha1);
	Close_widget(up_win); 
	return (0);

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
			categorie = strdup(categories[j]);
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

	tmp = crv_strdup(sChemin);
	truename = basename((const char *)tmp);
	crv_free(tmp);
	if (truename == NULL)
	{
		g_free(sChemin);
		crv_free(sha1);
		crv_free (grp);
		g_free(sBuffer);
		crv_free (categorie);
		Close_widget(up_win);
		return;
	}

	/* Ecriture du fichier d'info */
	fprintf(stdout, "%s%s%s\n", "name=", truename,"");
	fprintf(stdout, "%s%s%s\n", "group=", grp,"");
	fprintf(stdout, "%s%s%s\n", "cat=", categorie, "");
	if (strlen(sBuffer) > 0) {
		fprintf(stdout, "%s", sBuffer);
	}
	/* Fin de l'ecriture */

	pid_t pid;
	
	
	if ((pid = fork()) < 0)
	{
		printf("fork failed\n");
		return;
	}

	if (pid == 0)
	{
		Put(db, sChemin, server, grp, NULL, categorie, sBuffer);
		
		crv_free (grp);
		crv_free (categorie);
		g_free(sChemin);
		/*
		crv_free(sha1);
		crv_free(server);
		*/
		exit(EXIT_SUCCESS);
	}
	else
	{
		g_free(sChemin);
		Close_widget(up_win);
	return;
	}
	
	return;
}


static char *Msg_sha1;
static int Msg_cb (void *Box, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
      if (!crv_strncmp(azColName[i], "Sha1"))
        Msg_sha1 = crv_strdup(argv[i]);
    }
    return 0;
}


static void Valid_msg(GtkWidget *widget, gpointer data)
{
	GSList *pList;
	sqlite3 *db = NULL;
	const gchar *sLabel = NULL;
	char Description[SIZE];
	char *id_msg = NULL;
	char *req = NULL;
	char *zErrMsg = 0;
    int rc = -1;
    Msg_sha1 = NULL;

	id_msg = data;

	/***************************************/
	/* Section recuperation du commentaire */
	/***************************************/
	GtkTextBuffer* pTextBuffer;
	GtkTextIter iStart;
	GtkTextIter iEnd;
	gchar* sBuffer;
	pTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msg_View));
	/* On recupere l'origine du buffer */
	gtk_text_buffer_get_start_iter(pTextBuffer, &iStart);
	/* On recupere la fin du buffer */
	gtk_text_buffer_get_end_iter(pTextBuffer, &iEnd);

	/* On copie le contenu du buffer dans une variable */
	sBuffer = gtk_text_buffer_get_text(pTextBuffer, &iStart, &iEnd, TRUE);
	/***********************************/
	/* Fin recuperation du commentaire */
	/***********************************/

	if (strlen(sBuffer) == 0) {
		Warning ("\nCommentaire manquant !");
		return;
	}


	rc = sqlite3_open( "Database.db", &db);
  if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return;
	}
  
	Msg(db, id_msg, sBuffer);
	sqlite3_close(db);
	Close_widget(msg_win);
	return;
}

/* Fonction qui ouvre une fenetre permettant d'uploader un fichier */
static void Upload_file(GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	GtkWidget *pVBox;
	GtkWidget *HBox_fileselect;
	GtkWidget *file_select;
	GtkWidget *pSeparator;
	GtkWidget *pButton[1];
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


	for (i=0; categories[i]; i++)
	{
		if (!strstr(categories[i], "\n")) {
			gtk_combo_box_insert_text(GTK_COMBO_BOX(cat_list_combobox), i, categories[i]);
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
	//pButton[1] = gtk_button_new_with_label("Annuler");

	/* Ajout des boutons dans la boite horizontale */
	gtk_box_pack_start(GTK_BOX(HBox), pButton[0], FALSE, TRUE, 0);
  //gtk_box_pack_start(GTK_BOX(HBox), pButton[1], FALSE, TRUE, 0);

	//g_signal_connect(G_OBJECT(pButton[1]), "clicked", G_CALLBACK(gtk_widget_destroy), (GtkWidget*)up_win);
	g_signal_connect(G_OBJECT(pButton[0]), "clicked", G_CALLBACK(Valid_upload), NULL);

	/* Affichage de la fenetre */
	gtk_widget_show_all(up_win);
}

static void pre_Upload_file (GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	pthread_attr_t *thread_attributes;
  pthread_t tid;
	/* Initialisation des attributs d'un futur thread pour qu'il soit détaché */
      if (NULL == (thread_attributes = malloc (sizeof *thread_attributes)))
        {
          fprintf (stderr, "Problème avec malloc()\n");
          exit (EXIT_FAILURE);
        }
      if (0 != pthread_attr_init (thread_attributes))
        {
          fprintf (stderr, "Problème avec pthread_attr_init()\n");
          free (thread_attributes);
          exit (EXIT_FAILURE);
        }
      if (0 !=
          pthread_attr_setdetachstate (thread_attributes,
                                       PTHREAD_CREATE_DETACHED))
        {
          fprintf (stderr, "Problème avec pthread_attr_setdetachstate()\n");
          free (thread_attributes);
          exit (EXIT_FAILURE);
        }
		/* Création du thread */
      if (0 !=
            pthread_create (&tid, thread_attributes, Upload_file, NULL))
        {
          fprintf (stderr, "Problème avec pthread_create()\n");
          free (thread_attributes);
          exit (EXIT_FAILURE);
        }
/* Libération des ressources nécessaires pour les attributs du thread */
      pthread_attr_destroy (thread_attributes);
}

/* Fonction qui ouvre une fenetre permettant de poster un commentaire */
static void Send_msg(GtkWidget *widget, GtkTreeViewColumn *pColumn)
{
	GtkWidget *pVBox;
	GtkWidget *HBox_fileselect;
	GtkWidget *file_select;
	GtkWidget *pSeparator;
	GtkWidget *pButton[1];
	GtkWidget *HBox;
	GtkWidget *pScrollbar;
	GtkWidget *pFrame;

	gchar *buff;
	FILE *fd = NULL;

	int i;


	/* Creation de la fenetre */
  msg_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  /* Definition de la position */
	gtk_window_set_position(GTK_WINDOW(msg_win), GTK_WIN_POS_CENTER);
	/* Definition de la taille de la fenetre */
  gtk_window_set_default_size(GTK_WINDOW(msg_win), 480, 0);
	gtk_container_set_border_width(GTK_CONTAINER(msg_win), 4);
  /* Titre de la fenetre */
  gtk_window_set_title(GTK_WINDOW(msg_win), "Post message");

  /* Connexion du signal "destroy" */
  g_signal_connect(G_OBJECT(msg_win), "destroy", G_CALLBACK(Close_widget), (GtkWidget*) msg_win);

	/* Creation d'un VBox */
  pVBox = gtk_vbox_new(FALSE, 0);
	/* Ajout de la VBox a la fenetre principale */
	gtk_container_add(GTK_CONTAINER(msg_win), pVBox);


	/****************************************************/
	/* Creation de la zone de texte pour le commentaire */
	/****************************************************/

	/* Creation du premier GtkFrame */
	buff = g_locale_to_utf8("Commentaire", -1, NULL, NULL, NULL);
  pFrame = gtk_frame_new(buff);
  g_free(buff);
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, TRUE, 0);

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(pFrame),pScrollbar);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	msg_View = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(pScrollbar), msg_View);
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
	buff = g_locale_to_utf8("Envoyer", -1, NULL, NULL, NULL);
	pButton[0] = gtk_button_new_with_label(buff);
	g_free(buff);
	/*
	buff = g_locale_to_utf8("Annuler", -1, NULL, NULL, NULL);
	pButton[1] = gtk_button_new_with_label(buff);
	g_free(buff);
	*/

	/* Ajout des boutons dans la boite horizontale */
	gtk_box_pack_start(GTK_BOX(HBox), pButton[0], FALSE, TRUE, 0);
  //gtk_box_pack_start(GTK_BOX(HBox), pButton[1], FALSE, TRUE, 0);

	//g_signal_connect(G_OBJECT(pButton[1]), "clicked", G_CALLBACK(gtk_widget_destroy), (GtkWidget*)msg_win);
	//g_signal_connect(G_OBJECT(pButton[1]), "clicked", G_CALLBACK(Close_widget), (GtkWidget*)msg_win);
	g_signal_connect(G_OBJECT(pButton[0]), "clicked", G_CALLBACK(Valid_msg), choice_in_list);

	/* Affichage de la fenetre */
	gtk_widget_show_all(msg_win);
}


static void pre_Sync()
{
	char *zErrMsg = 0;
	int rc = -1;
	int i;
	
	sqlite3_close(db);
	(void)unlink("Database.db");

	gtk_tree_store_clear(pTreeStore);

	gtk_combo_box_remove_text(GTK_COMBO_BOX(ListBox), 0);
	for (i=0; categories[i]; i++) {
		gtk_combo_box_remove_text(GTK_COMBO_BOX(ListBox), 0);
		if(categories[i] != NULL)
			crv_free(categories[i]);
	}
	if (categories != NULL)
		crv_free(categories);

	Sync();
	rc = sqlite3_open("Database.db", &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}
	List();
	
	gen_combo_box_list();
	gtk_combo_box_set_active (GTK_COMBO_BOX(ListBox), 0);
	return;
}

/* Generate Gtk GUI */
void gen_Gtk_gui(int argc, char **argv, sqlite3 *main_db)
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
  db = main_db;
	choice_in_list = NULL;

	gtk_init(&argc, &argv);

  /* Creation de la fenetre principale
	 * pWindows = principale Windows
	 */
  pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(pWindow), "gCreuvux v0.73 ");
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
      G_CALLBACK(pre_Sync),
      pColumn,
      -1);

	// Creation du bouton INFO
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_PROPERTIES,
      "En savoir plus sur le fichier selectionne",
      NULL,
      G_CALLBACK(Info),
      pColumn,
      -1);

	// Creation du bouton NEW (command DIFF)
    /*
  gtk_toolbar_insert_stock(GTK_TOOLBAR(pToolbar),
      GTK_STOCK_NEW,
      "Afficher les nouveautes",
      NULL,
      G_CALLBACK(NULL),
      pColumn,
      -1);
    */


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
	gen_group_list();

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
  g_signal_connect(G_OBJECT(pEntry), "activate", G_CALLBACK(Find), NULL);

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
	gchar* sUtf8;



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
	sUtf8 = g_locale_to_utf8("Liste Distante", -1, NULL, NULL, NULL);
    pTabLabel = gtk_label_new(sUtf8);
	gtk_notebook_append_page(GTK_NOTEBOOK(pNotebook), pScrollbar, pTabLabel);
    g_free(sUtf8);
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
	sUtf8 = g_locale_to_utf8("Download/Upload", -1, NULL, NULL, NULL);
    pTabLabel = gtk_label_new(sUtf8);
	gtk_notebook_append_page(GTK_NOTEBOOK(pNotebook), load_scrollbar, pTabLabel);
	g_free(sUtf8);

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
    List();
	g_timeout_add_seconds ((guint)1, (GSourceFunc)Print_load_progress, NULL);

	gtk_main();
}
