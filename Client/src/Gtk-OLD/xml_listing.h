/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef XML_LISTING_H
#define XML_LISTING_H

extern int xsltransform ( const char *, const char *, int);

#include <libxml/tree.h>
extern xmlDocPtr xmldoc;

extern void download_dom_in (void);
extern void download_dom_out (void);
extern void list (void);
extern void list_categorie(void);
extern void list_group(void);
extern void Info(const char *);
extern int	return_max_id (void);
extern char *list_group_for_upload(void);
extern char *list_categorie_for_upload(void);
extern void Find(const char *);
extern char *Find_id_with_filename(const char *);

#endif /* XML_LISTING_H */
