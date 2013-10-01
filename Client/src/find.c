/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SQLITE3 Stuff */
#include <sqlite3.h>

#include "creuvard.h"

static int max_name;
static int max_rawid;
static int max_size;

static int init_table (void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    switch (i)
    {
      case 0:
        max_rawid = atoi(argv[i]);
        break;
      case 1:
        max_name = atoi(argv[i]);
        break;
      case 2:
        max_size = atoi(argv[i]);
        break;
    }
  }
  return 0;
}

static void print_padding (int nb)
{
  int i;
  for (i=0; i<nb; i++)
    fprintf(stdout, "%s", " ");
  return;
}

static void print_end_table(void)
{
  int i;
  printf("+");
  for (i=0; i<max_rawid+2; i++)
    printf("-");
  printf("+");
  for (i=0; i<max_name+2; i++)
    printf("-");
  printf("+");
  for (i=0; i<max_size+2; i++)
    printf("-");
  printf("+\n");

  return;
}


static void print_headers_table(void)
{
  int i;
  int diff;
  printf("+");
  for (i=0; i<max_rawid+2; i++)
    printf("-");
  printf("+");
  for (i=0; i<max_name+2; i++)
    printf("-");
  printf("+");
  for (i=0; i<max_size+2; i++)
    printf("-");
  printf("+\n");

  printf("| Id ");
  diff = max_rawid - strlen("Id");
  if (diff >= 0 )
      print_padding (diff);


  printf("| Name ");
  diff = max_name - strlen("Name");
  if (diff >= 0 )
      print_padding (diff);

  printf("| Size ");
  diff = max_size - strlen("Size");
  if (diff >= 0 )
      print_padding (diff);
  printf("|");

  printf("\n+");
  for (i=0; i<max_rawid+2; i++)
    printf("-");
  printf("+");
  for (i=0; i<max_name+2; i++)
    printf("-");
  printf("+");
  for (i=0; i<max_size+2; i++)
    printf("-");
  printf("+\n");


  return;
}



static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  int diff;

  for(i=0; i<argc; i++){
    printf("|");
		if (i == 2)
		{
			char *ep;
			char mo[SIZE];
			long lval;
			int j;
			
			/* Size operation */
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
			diff = max_size - strlen(mo);	
			fprintf(stdout, " %s ", mo);
		}
		else
			fprintf(stdout, " %s ", argv[i]);
    
		switch (i)
    {
      case 0:
        diff = max_rawid - strlen(argv[i]);
        break;
      case 1:
        diff = max_name - strlen(argv[i]);
        break;
    }
    if (diff >= 0 )
      print_padding (diff);

  }

  fprintf(stdout, "%s", "|\n");
  return 0;
}




int Find(sqlite3 *db, const char *keyword)
{
	char *zErrMsg = 0;
	int rc = -1;
	char req[SIZE];
	max_name = 0;
  max_rawid = 0;
  max_size = 0;

  rc = sqlite3_exec(db, "SELECT max(length(rowid)),max(length(Name)),max(length(Size)) FROM Files;" , init_table, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

	(void)crv_strncpy(req, "SELECT DISTINCT rowid,Name,Size FROM Files WHERE Name like '%", sizeof(req));
	(void)crv_strncat(req, keyword, sizeof(req));
	(void)crv_strncat(req, "%';", sizeof(req));
  print_headers_table();
   rc = sqlite3_exec(db, req , callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  print_end_table();	
	return (0);
}
