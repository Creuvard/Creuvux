/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>


/* SSL stuff */
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h> 
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

/* ZLIB Stuff */
#include <zlib.h>


/*
 * SQLite3 Stuff
 */
#include <sqlite3.h>


#include "creuvard.h"
#include "network.h"
#include "log.h"
#include "server_conf.h"
#include "SSL_sendfile.h"
#include "get.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);


/* Command options. */
extern Command_options command;

static int pre_Get_cb (void *db, int argc, char **argv, char **azColName){
  int i;

  /* insert into listing (sha1,name,size,path) values ('SHA1', 'FILE', 'SIZE', 'PATH'); */

  for(i=0; i<argc; i++){
    /*
		printf("(%d) %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
		*/
		if (!strcmp((const char *)azColName[i], "Name"))
      command.filename = strdup(argv[i]);

		if (!strcmp((const char *)azColName[i], "Path"))
      command.path = strdup(argv[i]);
  }
	
	
  return 0;
}

int pre_Get(char **c)
{
	char *ep = NULL;
	long lval = 0;
	sqlite3 *db = NULL;
	char *zErrMsg = 0;
	int rc = -1;
	char req[SIZE];


	errno = 0;
	
	command.name = crv_strdup("GET");
	command.sha1 = crv_strdup(c[2]);
	
	lval = strtol(c[3], &ep, 10);
	
	if (c[3][0] == '\0' || *ep != '\0') {
		Log ( "CRASH", 0, "%s\n", "Command format is wrong, begin is not a number");
		return (-1);
	}
	
	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || (lval > INT_MAX || lval < INT_MIN))
	{
		Log ( "CRASH", 0, "%s\n", "Command format is wrong, begin is out of the rang");
		return (-1);
	}
	
	command.begin = (off_t)lval;
	/* Transform c[3] and c[4] in off_t */
	lval = strtol(c[4], &ep, 10);
	
	if (c[4][0] == '\0' || *ep != '\0') {
		Log ( "CRASH", 0, "%s\n", "Command format is wrong, end is not a number");
		return (-1);
	}
	
	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) || (lval > INT_MAX || lval < INT_MIN))
	{
		Log ( "CRASH", 0, "%s\n", "Command format is wrong, end is out of the rang");
		return (-1);
	}
	
	command.end = (off_t) lval;
	/*
	 * * Faut vérifier que le client à le droit de télécharger le fichier et 
	 * * ensuite SEULEMENT le lui envoyer.
	 * */
	/*
	result = get_grp_user(command.user);
	if (result == -1) {
		Log ( "CRASH", 0, "%s\n", "Can't get group of user");
		return (-1);
	}
	*/
	/* Open main database */
	rc = sqlite3_open( "/.creuvux.db", &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return (-1);
	}

	/* Get  info about file wanted by client */
	(void)snprintf(req, sizeof(req), "SELECT Name,Path FROM Files WHERE Sha1 = '%s';", command.sha1);
  
	rc = sqlite3_exec(db, req , pre_Get_cb, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	sqlite3_close(db);	
	
	/* Get PATH of file and set command.path */
	/*
	result = Set_command_info (NULL);
	if (result == -1) {
		Log ( "CRASH", 0, "%s\n", "Can't set command about user");
		return (-1);
	}
	*/
	return (0);
}


int Get(SSL *ssl, const char *filename, off_t begin, off_t end, const char *path)
{
	int result = -1;
	Log ("OK", 1, "GET");
	
	/* Obtain filename with sha1 */
	result = SSL_write (ssl, "GET_ACK", strlen("GET_ACK"));
	if (result <= 0) {
		Log ("WARNING", 1, "GET_ACK -> PB");
		return (-1);
	}
	
	Log ("INFO", 1, " Title = %s",  filename);
	Log ("INFO", 1, " Begin = %ld", begin);
	Log ("INFO", 1, " End   = %ld", end);
	
	result = crv_chdir(path);
	if (result == -1 ) {
		Log ("WARNING", 1, "traitement_conec(): crv_chdir() failed");
		return (-1);
	}
	
	result = SSL_sendfile( ssl, command.filename , command.begin, command.end);
	if (result == -1 ) {
	Log ("WARNING", 1, "SSL_sendfile(): Send 'command.filename' failed");
	return (-1);
	}
	else {
		Log ("OK", 1, "GET -> OK");
	}
	
	result = SSL_write (ssl, "GET_END", strlen("GET_END"));
	if (result <= 0) {
	Log ("WARNING", 1, "GET_END -> PB");
	return (-1);
	}
	return (0);
}
