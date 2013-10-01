/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef GTK_GET_H
#define GTK_GET_H

#include <libxml/tree.h>
#include <gtk/gtk.h>
extern xmlDocPtr xmldoc_dl;


extern GtkTreeStore *load_tree;

extern int  Gtk_get(const char *);
extern int Init_download_tree(void);
extern void Set_download_tree(void);
extern void Free_download_tree(void);
extern void add_file_for_dl (const char *);
extern void Load(GtkTreeStore *);
extern void Load1(void);


#endif /* GTK_GET_H */
