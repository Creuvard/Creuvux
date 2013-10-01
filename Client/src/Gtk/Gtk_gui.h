/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef GTK_GUI_H
#define GTK_GUI_H
#include <gtk/gtk.h>
#include "sqlite3.h"

extern GtkWidget		*pWindow;
extern GtkWidget		*pNotebook;
extern void gen_Gtk_gui(int , char **,sqlite3 *);


#endif /* GTK_GUI_H */
