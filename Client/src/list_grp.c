/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
/* SQLite stuff */
#include <sqlite3.h>


#include "sync.h"
#include "creuvard.h"
#include "client_conf.h"
#include "thread.h"
#include "help.h"
#include "network.h"
#include "list.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);




char **grp;
static int step;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
		grp[step] = crv_strdup(argv[i]);
	}
	step += 1;
  return 0;
}

char *List_group_for_upload(sqlite3 *db)
{
	char *groupe = NULL;
	char *zErrMsg = NULL;
	char *ep = NULL;
	char buf[SIZE];
	int rc = -1;
	int lval;
	int i;

	step = 0;
	if (NULL == (grp = malloc (SIZE * sizeof *grp)))
    return;

	rc = sqlite3_exec(db, "SELECT DISTINCT groupname FROM Grp_Sha1;", callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
		crv_free(grp);
  }	

	couleur ("33;32");
	for(i=0; i<step; i++) {
		fprintf(stdout, "[%d] %s%s", i, grp[i], "\n");
	}
	couleur ("0");
	fprintf(stdout, "Entrez votre choix: ('Entree' si aucun choix ne vous satisfait): ");
	fflush(stdout);
	
	if (fgets(buf, sizeof(buf), stdin) != NULL)
		buf[strcspn(buf, "\n")] = '\0';
	
	lval = (int)strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		for (i=0; grp[i]; i++)
			crv_free(grp[i]);
		crv_free(grp);
		return NULL;
	}
	if (lval > i-1) {
		fprintf(stderr, "%s\n", "Out of range");
		for (i=0; grp[i]; i++)
			crv_free(grp[i]);
		crv_free(grp);
		return NULL;
	}


	printf("on a la groupe '%s'\n", grp[lval]);
	groupe = crv_strdup(grp[lval]);
	return groupe;
	
}

void List_group(sqlite3 *db)
{
	char *zErrMsg = 0;
	char *ep = NULL;
	char *req = NULL;
	char buf[SIZE];
	int lval;
	int rc = -1;
	int i;
	step = 0;

	if (NULL == (grp = malloc (SIZE * sizeof *grp)))
    return;

	rc = sqlite3_exec(db, "SELECT DISTINCT groupname FROM Grp_Sha1;", callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
		crv_free(grp);
  }	

	couleur ("33;32");
	for(i=0; i<step; i++) {
		fprintf(stdout, "[%d] %s%s", i, grp[i], "\n");
	}
	couleur ("0");
	fprintf(stdout, "Entrez votre choix: ('Entree' si aucun choix ne vous satisfait): ");
	fflush(stdout);
	
	if (fgets(buf, sizeof(buf), stdin) != NULL)
		buf[strcspn(buf, "\n")] = '\0';
	
	lval = (int)strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		fprintf(stderr, "%s\n", "Argument is not a number");
		for (i=0; grp[i]; i++)
			crv_free(grp[i]);
		crv_free(grp);
		return;
	}
	if (lval > i-1) {
		fprintf(stderr, "%s\n", "Out of range");
		for (i=0; grp[i]; i++)
			crv_free(grp[i]);
		crv_free(grp);
		return;
	}

	rc = sqlite3_exec(db, "SELECT max(length(rowid)),max(length(Name)),max(length(Size)) FROM Files;" , init_table, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
	
	couleur ("33;32");
	print_headers_table();

	req = sqlite3_mprintf("SELECT DISTINCT Files.rowid,Files.Name,Files.Size FROM Files  WHERE Sha1 IN (SELECT Sha1 FROM Grp_Sha1 WHERE groupname='%q');", grp[lval]);
	rc = sqlite3_exec(db, req, List_callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
		crv_free(grp);
  }
	sqlite3_free(req);
	print_end_table();
	couleur ("0");


	for (i=0; i<step; i++)
		if (grp[i]!= NULL)
			crv_free(grp[i]);
	if (grp[i]!= NULL)
		crv_free(grp);
	step = 0;

	return;
}
