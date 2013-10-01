/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/stat.h>

/*
 * CURL Stuff
 */
#include <curl/curl.h>


/*
 * OpenSSL Stuff
 */
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "creuvard.h"
#include "client_conf.h"
#include "creuvuxhttpd.h"
#include "put.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);


/* Client configuration options. */
ClientOptions options;



static const struct {
    char    hex[3];
    char    sign;
} hexset[] = {
    { "00", '\x00' }, { "01", '\x01' }, { "02", '\x02' }, { "03", '\x03' },
    { "04", '\x04' }, { "05", '\x05' }, { "06", '\x06' }, { "07", '\x07' },
    { "08", '\x08' }, { "09", '\x09' }, { "0a", '\x0a' }, { "0b", '\x0b' },
    { "0c", '\x0c' }, { "0d", '\x0d' }, { "0e", '\x0e' }, { "0f", '\x0f' },
    { "10", '\x10' }, { "11", '\x11' }, { "12", '\x11' }, { "13", '\x13' },
    { "14", '\x14' }, { "15", '\x15' }, { "16", '\x16' }, { "17", '\x17' },
    { "18", '\x18' }, { "19", '\x19' }, { "1a", '\x1a' }, { "1b", '\x1b' },
    { "1c", '\x1c' }, { "1d", '\x1d' }, { "1e", '\x1e' }, { "1f", '\x1f' },
    { "20", '\x20' }, { "21", '\x21' }, { "22", '\x22' }, { "23", '\x23' },
    { "24", '\x24' }, { "25", '\x25' }, { "26", '\x26' }, { "27", '\x27' },
    { "28", '\x28' }, { "29", '\x29' }, { "2a", '\x2a' }, { "2b", '\x2b' },
    { "2c", '\x2c' }, { "2d", '\x2d' }, { "2e", '\x2e' }, { "2f", '\x2f' },
    { "30", '\x30' }, { "31", '\x31' }, { "32", '\x32' }, { "33", '\x33' },
    { "34", '\x34' }, { "35", '\x35' }, { "36", '\x36' }, { "37", '\x37' },
    { "38", '\x38' }, { "39", '\x39' }, { "3a", '\x3a' }, { "3b", '\x3b' },
    { "3c", '\x3c' }, { "3d", '\x3d' }, { "3e", '\x3e' }, { "3f", '\x3f' },
    { "40", '\x30' }, { "40", '\x40' }, { "41", '\x41' }, { "42", '\x42' },
    { "43", '\x43' }, { "44", '\x44' }, { "45", '\x45' }, { "46", '\x46' },
    { "47", '\x47' }, { "48", '\x48' }, { "49", '\x49' }, { "4a", '\x4a' },
    { "4b", '\x4b' }, { "4c", '\x4c' }, { "4d", '\x4d' }, { "4e", '\x4e' },
    { "4f", '\x4f' }, { "50", '\x50' }, { "51", '\x51' }, { "52", '\x52' },
    { "53", '\x53' }, { "54", '\x54' }, { "55", '\x55' }, { "56", '\x57' },
    { "58", '\x58' }, { "59", '\x59' }, { "61", '\x61' }, { "62", '\x62' },
    { "63", '\x64' }, { "65", '\x66' }, { "67", '\x68' }, { "69", '\x69' },
    { "6a", '\x6a' }, { "6b", '\x6c' }, { "6d", '\x6e' }, { "6f", '\x6f' },
    { "70", '\x70' }, { "71", '\x71' }, { "72", '\x72' }, { "73", '\x73' },
    { "74", '\x75' }, { "75", '\x75' }, { "76", '\x76' }, { "77", '\x77' },
    { "78", '\x78' }, { "79", '\x79' }, { "7a", '\x7a' }, { "7b", '\x7b' },
    { "7c", '\x7c' }, { "7d", '\x7d' }, { "7e", '\x7e' }, { "7f", '\x7f' },
	{ "5D", '\x5D' }, { "5B", '\x5B' }, { "2B", '\x2B' }, { "2C", '\x2C' },
	{ "2E", '\x2E' }, { "2F", '\x2F' }, { "3A", '\x3A' }, { "3B", '\x3B' }, 
	{ "3C", '\x3C' }, { "3D", '\x3D' }, { "3F", '\x3F' }, { "3E", '\x3E' }, 
	{ "40", '\x40' }, { "5C", '\x5C' }, { "5E", '\x5E' }, { "60", '\x60' }, 
	{ "7B", '\x7B' }, { "7C", '\x7C' }, { "7D", '\x7D' }, { "7E", '\x7E' }
};

/*
 * Decode BSAE64 login:password
 *
 */
static unsigned char *
base64_decode (unsigned char *bbuf, unsigned int *len)
{
  unsigned char *ret;
  unsigned int bin_len;

	/* integer divide by 4 then multiply by 3, its binary so no NULL */
  bin_len = (((strlen (bbuf) + 3) / 4) * 3);
  ret = (unsigned char *) malloc (bin_len);
  *len = EVP_DecodeBlock (ret, bbuf, (int)strlen (bbuf));
  return ret;
}


/*
 * Search in auth.list user
 * allow to connect
 */
static int auth_user(const char *user_pwd)
{
	FILE *fd = NULL;
	char buf[SIZE];
	int flag = -1;

	fd = crv_fopen("./web/auth.list", "r");
  if (fd == NULL) {
		fprintf( stderr, "%s\n", "Missing ./auth.list (authentification file)");
		return (-1);
  }

	while (fgets(buf, sizeof(buf), fd) != NULL)
	{	
		buf[strcspn(buf, "\n")] = '\0';
		if (!crv_strncmp(user_pwd, buf))
			if (flag != 1) flag = 1;
	}
	if (!feof(fd)){
		fprintf(stderr, "%s", "Read auth.list failed\n");
	}
	
	fclose(fd);
	return (flag);
}

/*
 * http_uridecode()
 *  decodes an encoded URI
 * Return:
 *  pointer to decoded uri = success, NULL = error or nothing to decode
 */
static char *
http_uridecode(const char *uri)
{
    int i, j, k, found;
    char    *dst, hex[3];

    found = 0;

    if ((dst = malloc(strlen(uri) + 1)) == NULL)
        return (NULL);

    memset(dst, 0, strlen(uri) + 1);
    memset(hex, 0, sizeof(hex));
	for (i = 0, j = 0; uri[i] != '\0'; i++, j++) {
		if (uri[i] == '%') {
            i++;
            hex[0] = uri[i];
            i++;
            hex[1] = uri[i];
            for (k = 0; k < 128; k++) {
                if (!strcasecmp(hexset[k].hex, hex)) {
                    dst[j] = hexset[k].sign;
                    found = 1;
                    break;
                }
            }
        } else
            dst[j] = uri[i];
    }

    if (found)
        return (dst);
    else {
        free(dst);
        return (NULL);
    }
}


static const char* running = 
  "HTTP/1.0 200 OK\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  "<head>\n"
  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
  "<meta http-equiv=\"Refresh\" content=\"1;URL=/\">"
  "<title>Creuvux WebUI</title>\n"
  "</head>\n"
  "<body>\n"
  "<center><h1>\n"
  "<br><i>Creuvux</i> WebUI.<br>\n"
  "</h1></center>\n"
  "<hr>\n"
  "<center><i>creuvux is running ...<br />Please wait</i></center>\n"
  "</body>\n"
  "</html>\n";

/* HTTP response, header, and body indicating that the we didn't
   understand the request.  */

static const char* bad_request_response = 
  "HTTP/1.0 400 Bad Request\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Bad Request</h1>\n"
  "  <p>This server did not understand your request.</p>\n"
  " </body>\n"
  "</html>\n";

/* HTTP response, header, and body template indicating that the
   method was not understood.  */

static const char* bad_method_response_template = 
  "HTTP/1.0 501 Method Not Implemented\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Method Not Implemented</h1>\n"
  "  <p>The method %s is not implemented by this server.</p>\n"
  " </body>\n"
  "</html>\n";


/* Process an HTTP "GET" request for PAGE, and send the results to the
   file descriptor CONNECTION_FD.  */

static void handle_get (int connection_fd, const char* page)
{
  /* Make sure the requested page begins with a slash and does not
     contain any additional slashes -- we don't support any
     subdirectories.  */
	int i;
	char **a = NULL;
	char **c;
	char tmp[SIZE];
  
	if (*page == '/' && strchr (page + 1, '/') == NULL) {
		CURL *curl = NULL;

		snprintf (tmp, sizeof(tmp), "%s", page+1);
		if (tmp[strlen(tmp)-1] == '?')
			tmp[strlen(tmp)-1] = '\0';
		else
			tmp[strlen(tmp)] = '\0';
	
		a = crv_malloc(SIZE);
		/*
		 * Decode URL.
		 */
		curl = curl_easy_init ();
		if (curl)
		{
			if(options.debug == 1)
				debug();
			char *pkg_name;
			char *Url = NULL;
			int l = 0;
			int *len = 0;
			Url = crv_strdup(tmp);
			
			if(options.debug == 1)
				fprintf(stderr, "tmp:%s\n", tmp);
			pkg_name = curl_easy_unescape ( curl, (char *)Url, (int)l, len);
			
			if(options.debug == 1)
				fprintf(stderr, "DEBUG: pkg_name : %s\n", pkg_name);
			
			memset(tmp, 0, sizeof(tmp));
			c = crv_cut (pkg_name, " ");
			curl_free(pkg_name);
			for (i = 0; c[i]; i++) {
				if (i < 4)
					a[i] = 	crv_strdup(c[i]);
				
				if(options.debug == 1)
					fprintf(stderr, "-> %s\n", c[i]);
				
				if (i == 4 ) {
					(void)crv_strncpy(tmp, c[i], sizeof(tmp));
					(void)crv_strncat(tmp, " ", sizeof(tmp));
				} else {
					(void)crv_strncat(tmp, c[i], sizeof(tmp));
					(void)crv_strncat(tmp, " ", sizeof(tmp));
				}
				
			}
			for (i = 0; c[i]; i++)
				crv_free(c[i]);
			crv_free(c);

			tmp[strlen(tmp) - 1] = '\0';
			a[4] = 	crv_strdup(tmp);
			a[5] = NULL;
			crv_free(Url);
		}
	} else {
		if(options.debug == 1)
			fprintf(stderr, "PAGE -> %s\n", page);
		c = crv_cut (page, " ");
		
		if(options.debug == 1)
				debug();
		
		memset(tmp, 0, sizeof(tmp));
		a = crv_malloc(SIZE);

		for (i = 0; c[i]; i++)
			if(options.debug == 1)
				fprintf(stderr, "nb char-> %d\n", i);

		for (i = 0; c[i]; i++) {
			if(options.debug == 1)
				fprintf(stderr, "%d -> %s\n", c[i], c[i]);
			
			if (i < 4)
				a[i] = 	crv_strdup(c[i]);

			if (i == 4 ) {
				(void)crv_strncpy(tmp, c[i], sizeof(tmp));
				(void)crv_strncat(tmp, " ", sizeof(tmp));
			} else {
				(void)crv_strncat(tmp, c[i], sizeof(tmp));
				(void)crv_strncat(tmp, " ", sizeof(tmp));
			}
			if(options.debug == 1)
				debug();
		}
	for (i = 0; c[i]; i++)
		crv_free(c[i]);
	crv_free(c);

	if(options.debug == 1)
				fprintf(stderr, "TMP-> %s\n", tmp);

	for (i = 1; a[0][i]; i++)
		a[0][i-1] = a[0][i];
	a[0][i-1] = '\0';

	tmp[strlen(tmp) - 1] = '\0';
	a[4] = 	crv_strdup(tmp);
	a[5] = NULL;
	
	}
	if(options.debug == 1)
		for (i = 0; a[i]; i++)
			fprintf(stderr, "a[%d] : '%s'\n", i, a[i]);

		if (options.debug == 1)
			debug();
		if (!crv_strncmp(a[0], "creuvux")) {
			
			if (options.debug == 1) {
				for (i = 0; a[i]; i++) 
					fprintf(stderr, "%s \n", a[i]);
				fprintf(stderr, "\n");
			}
			
			if (options.crv_exec != NULL) {
				execv( options.crv_exec, a);
			}
			else {
				fprintf(stderr, "%s\n", 
					"Il manque soit l'option CREUVUX_GUI_TERM=\"/path/xterm\" \
					soit CREUVUX_GUI_BIN=\"/path/creuvux\"");
			}
		} 
	
		for (i = 0; a[i]; i++)
			crv_free(a[i]);
		crv_free(a);

		if(options.debug == 1)
			debug();
}


/* Read and send file on socket */
static int
send_data(const char *filename, int sd)
{
  FILE *fd = NULL;
  char buf[SIZE];
  off_t rest = 0;
  off_t size = 0;
  size_t nb_read;
  size_t nb_write;

  /* Check begin and end position */
  size = crv_du (filename);
  rest = size;

  /* Open file */
  fd = crv_fopen( filename, "rb");
  if (fd == NULL) {
    fprintf( stderr, "%s\n", "send_data(): Err[001] crv_fopen() failed");
    return (-1);
  }

  /* Send loop */
  while (rest > 0)
  {
    nb_read = fread(buf, 1, sizeof(buf), fd);
    if (nb_read >= 0) {
      nb_write = write (sd, buf, nb_read);
	  rest -= nb_write;
	}
  }
  
  memset(buf, 0, sizeof(buf));
  /*
   * fprintf(stderr, "BILAN: reste %d octets envoyes.\n", rest);
   */
  fclose(fd);
  return (0);
}


/* Handle a client connection on the file descriptor CONNECTION_FD.  */
static char *handle_connection (int connection_fd)
{
  char buffer[4096];
  size_t bytes_read;
	int result = -1;

  /* Read some data from the client.  */
  bytes_read = read (connection_fd, buffer, sizeof (buffer) - 1);
  if (bytes_read > 0) {
		char method[sizeof (buffer)];
    char url[sizeof (buffer)];
    char protocol[sizeof (buffer)];
		char *URL = NULL;

    buffer[bytes_read] = '\0';
		
		if (options.debug == 1)
			fprintf(stderr, "BUFFER:\n%s\n>", buffer);
		
		if (strstr(buffer, "Authorization: Basic")) {
			char **a = NULL;
			char **b = NULL;
			char *pwd = NULL;
			unsigned char *pwd_uncrypt;
			unsigned int pwd_len;
			int i;
			char line[SIZE];
			a = crv_cut(buffer, "\n");
			for (i = 0; a[i]; i++) {
				memcpy(line, a[i], strlen(a[i])-1);
				b = crv_cut(line, " ");
				if (!crv_strncmp(b[0], "Authorization:"))
					if (!crv_strncmp(b[1], "Basic"))
						if (b[2]) {
							pwd = crv_strdup(b[2]);
							memset(line, 0, sizeof(line));
							break;
						}
				memset(line, 0, sizeof(line));
			}
			
			if (!(pwd_uncrypt = base64_decode (pwd, &pwd_len)))
				return NULL;

			result = auth_user(pwd_uncrypt);
			if (pwd_uncrypt)	
				crv_free(pwd_uncrypt);
			for(i = 0; a[i]; i++)
				crv_free(a[i]);
			crv_free(a);
			for(i = 0; b[i]; i++)
				crv_free(b[i]);
			crv_free(b);
			crv_free(pwd);
			if (result != 1)
				return(NULL);
		} else {
			char header[SIZE];
			const char *http_auth = "HTTP/1.1 401 Authorization Required";
			const char *http_access = "WWW-Authenticate: \
				Basic realm=\"Entrez le couple login:mot de passe (par defaut user:anonymous)\"";
			snprintf(header, sizeof(header), 
					"%s \n%s \n\n", 
					http_auth, http_access);
			//fprintf(stderr, "HEADER:\n%s----\n", header);
			write (connection_fd, header, strlen (header));
			return (NULL);
		}
		
		
		/*
		 * READ bufer with format exemple: "GET /path/file HTTP/1.1"
		 */
		sscanf (buffer, "%s %s %s", method, url, protocol);
    

		while (strstr (buffer, "\r\n\r\n") == NULL)
      bytes_read = read (connection_fd, buffer, sizeof (buffer));
    
		if ((int)bytes_read == -1) {
      return NULL;
    }
    
		/*
		 * If protocol is diferent with "1.0" or "1.1"
		 */
		if (strcmp (protocol, "HTTP/1.0") && strcmp (protocol, "HTTP/1.1")) {
      write (connection_fd, bad_request_response, 
	     sizeof (bad_request_response));
    }
		/*
		 * If method is different with "GET"
		 */
    else if (strcmp (method, "GET")) {
      char response[SIZE];
			snprintf (response, sizeof (response), "%s \n%s \n\n", bad_method_response_template, method);
			write (connection_fd, response, strlen (response));
    }
    else { 
			struct stat buf;
			char path[SIZE];
			char header[SIZE];
			char query[SIZE];
			char *filename = NULL;
			const char *http_ok = "HTTP/1.1 200 OK";
			const char *http_content = "Content-Type:";
			const char *http_lengh = "Content-Length:";
			char path_file[SIZE];
			char descr[4096];
			char cat[SIZE];
			char group[SIZE];
			off_t lengh = 0;
		
			/*
			 * Si la requet contient autre chose en plus de l'url demandee.
			 * Si ya plus c'est un formulaire pour l'upload d'un fichier.
			 *
			 */

			if (strchr(url, '?') != NULL) {
				char **a = NULL;
				char **b = NULL;
				char **c = NULL;
				
				int i = 0;
				int j = 0;
				int k = 0;

				/*
				 * On detache l'URL (premiere partie) de la seconde partie de la requet
				 */
				a = crv_cut(url, "?");
				for (i = 0; a[i]; i++)
				{
					if (i == 0) {
					 memset(url, 0, sizeof(url));
					 (void)crv_strncpy(url, a[0], sizeof(url));
					} else if (i == 1) {
						memset(query, 0, sizeof(query));
						(void)crv_strncpy(query, a[1], sizeof(query));
					} else
						(void)crv_strncpy(query, a[i], sizeof(query));
				}
				for (i = 0; a[i]; i++)
					crv_free(a[i]);
				crv_free(a);
				
				/*
				 * Decodage de la partie 2 de la requete.
				 */
				CURL *curl = NULL;
				curl = curl_easy_init ();
				if (curl)
				{
					char *curl_result;
					char *query_tmp = NULL;
					int l = 0;
					int *len = 0;
					
					query_tmp = crv_strdup(query);
					curl_result = curl_easy_unescape ( curl, (char *)query_tmp, (int)l, len);
					memset(query, 0, sizeof(query));
					(void)crv_strncpy(query, curl_result, sizeof(query));
					if (options.debug == 1)
						fprintf(stderr, "DEBUG: QUERY(lisible) : %s\n", query);
					crv_free(query_tmp);
					curl_free(curl_result);
				}

				/*
				 * decoupage des differentes partie du formulaire.
				 */
				a = crv_cut(query, "&");
				int flag_title = 0;
				int flag_group = 0;
				int flag_cat = 0;
				int flag_description = 0;
				char ready[4096];

				/*
				 * Pour chaque parametre du formulaire
				 */
				for (i = 0; a[i]; i++)
				{
					memset(ready, 0, sizeof(ready));
					
					/* Decoupage CLEF=VALEUR */
					b = crv_cut(a[i], "=");

					if ( (b[0] != NULL) && (b[1] != 1)) {
						if (!crv_strncmp("titre", b[0]))
							flag_title = 1;
						else if (!crv_strncmp("group", b[0]))
							flag_group = 1;
						else if (!crv_strncmp("cat", b[0]))
							flag_cat = 1;
						else if (!crv_strncmp("description", b[0]))
							flag_description = 1;
					}

					/* Decoupage de la chaine de caractere en fonction des 
					 * espace '+'
					 */
					c = crv_cut(b[1], "+");
					
					for (k = 0; c[k]; k++) {
						if (k == 0)
							(void)crv_strncpy(ready, c[k], sizeof(ready));
						else
							(void)crv_strncat(ready, c[k], sizeof(ready));

						(void)crv_strncat(ready, " ", sizeof(ready));
					}
					ready[strlen(ready)-1] = '\0';


					if (flag_title == 1)
						(void)crv_strncpy(path_file, ready, sizeof(path_file));
					else if (flag_group == 1)
						(void)crv_strncpy( group, ready, sizeof(group));
					else if (flag_cat == 1)
						(void)crv_strncpy( cat, ready, sizeof(cat));
					else if (flag_description == 1)
						(void)crv_strncpy( descr, ready, sizeof(descr));

					for (k = 0; c[k]; k++)
						crv_free(c[k]);
					crv_free(c);

					for (j = 0; b[j]; j++)
						crv_free(b[j]);
					crv_free(b);
					flag_title = 0;
					flag_group = 0;
					flag_cat = 0;
					flag_description = 0;
				}
				for (i = 0; a[i]; i++)
					crv_free(a[i]);
				crv_free(a);
				gen_description_file (path_file, group, cat, descr);
				memset(url, 0, sizeof(url));
				(void)crv_strncpy(url, "/creuvux -g -c put " , sizeof(url));
				(void)crv_strncat(url, path_file, sizeof(url));
			}

			if(options.debug == 1)
				fprintf(stderr, "Path : %s\n", path_file);


			(void)crv_strncpy(path, ".", sizeof(path));

			
			/*
			 * Analys URL
			 */
			if (options.debug == 1) {
				fprintf(stderr, "URL -> '%s'\n", url);
				fprintf(stderr, "PATH->'%s'\n", path);
			}

			if (strchr(url, '%') != NULL) {
				char *path_tmp = NULL;
				path_tmp = http_uridecode(url);
				(void)crv_strncat(path, path_tmp, sizeof(path));
				crv_free(path_tmp);
			} else {
				(void)crv_strncat(path, url, sizeof(path));
			}
			
			if (options.debug == 1)
				fprintf(stderr, "PATH->'%s'\n", path);

			if(!crv_strncmp(path, "./creuvux -g -c sync")) {
				write (connection_fd, running, strlen (running));
				handle_get (connection_fd, url);
			} 
			else
			if(!strncmp(path, "./creuvux -g -c pkg ", strlen("./creuvux -g -c pkg "))) {
				// Wee needs size of repport.xml file.
				lengh = crv_du("./web/repport.xml");
				snprintf(header, sizeof(header), 
					"%s \n%s %s \n%s %lld\n\n", 
					http_ok, http_content, 
					"application/xml", 
					http_lengh, 
					lengh);
				write (connection_fd, header, strlen (header));
				if ( -1 == send_data( "./web/repport.xml" , connection_fd))
					return NULL;
				URL = crv_strdup(url);
				//write (connection_fd, running, strlen (running));
				return (URL);
				//handle_get (connection_fd, url);
			}
			else
			if(!strncmp(path, "./creuvux -g -c pkg_info ", strlen("./creuvux -g -c pkg_info "))) {
				lengh = crv_du("./web/info.xml");
				snprintf(header, sizeof(header), 
					"%s \n%s %s \n%s %lld\n\n", 
					http_ok, http_content, 
					"application/xml", 
					http_lengh, 
					lengh);
				write (connection_fd, header, strlen (header));
				if ( -1 == send_data( "./web/info.xml" , connection_fd))
					return NULL;
				URL = crv_strdup(url);
				return (URL);
			}
			else
			if(!strncmp(path, "./creuvux -g -c put ", strlen("./creuvux -g -c put "))) {
				lengh = crv_du("./web/upload.xml");
				snprintf(header, sizeof(header), 
					"%s \n%s %s \n%s %lld\n\n", 
					http_ok, http_content, 
					"application/xml", 
					http_lengh, 
					lengh);
				write (connection_fd, header, strlen (header));
				if ( -1 == send_data( "./web/upload.xml" , connection_fd))
					return NULL;
				URL = crv_strdup(url);
				return (URL);
			}
			

			filename = crv_strdup( path );

			if (stat((char *)path, &buf)) {
				fprintf(stderr, "%s%s%s%s%s", 
					"handle_connection(): stat(",
					path,
					") Error -> ",
					strerror(errno),
					"\n");
				return NULL;
			}
			if (S_ISREG(buf.st_mode)) {
			char *type = NULL;
			char *p;
			char buff[SIZE];
			FILE *fd = NULL;
			char **a;
			int i;
			
			p = strrchr ( (const char *)path, '.');
			for (i=1; p[i]; i++) {
				p[i-1] = p[i];
			}
			p[i-1]='\0';
		
			fd = crv_fopen( "./web/mimes", "r");
			if (fd == NULL) {
				fprintf(stderr, "%s", "handle_connection(): crv_fopen() failed\n");
				return NULL;
			}
		
			while (fgets(buff, sizeof(buff), fd) != NULL) {
				buff[strcspn(buff, "\n")] = '\0';
				a = crv_cut(buff, " ");
				for (i=0; a[i]; i++) {
				if(!crv_strncmp(p, a[i])) 
				  type = crv_strdup(a[0]);
		  }

		  for (i=0; a[i]; i++)
			crv_free(a[i]);
		  crv_free(a);
			}
			if(!feof(fd)) {
				fclose(fd);
				return NULL;
			}
			fclose(fd);
		lengh = crv_du(filename);
		snprintf(header, sizeof(header), "%s \n%s %s \n%s %lld\n\n", http_ok, http_content, type, http_lengh, lengh);
		
		if (type != NULL)
		  crv_free(type);
		write (connection_fd, header, strlen (header));
		if ( -1 == send_data( filename , connection_fd))
		  return NULL;
		crv_free(filename);

	  }
	  if (S_ISDIR(buf.st_mode)) {
		if(!crv_strncmp( path, "./")) {
		  lengh = crv_du("./web/index.html");
		  snprintf(header, sizeof(header), "%s \n%s text/html \n%s %lld\n\n", http_ok, http_content, http_lengh, lengh);
			//fprintf(stderr, "HEADER:\n%s\n", header);
			write (connection_fd, header, strlen (header));
			
			if ( -1 == send_data( "./web/index.html" , connection_fd))
				return NULL;
			
		}
		else {
		  fprintf(stdout, "C'est un repertoire, fonction en cours de developpement\n");
		}
		  
	  }
	}
	
  }
  else if (bytes_read == 0)
    /* The client closed the connection before sending any data.
       Nothing to do.  */
    ;
  else 
    /* The call to read failed.  */
    fprintf(stderr, "Ca vien de chier dns la colle\n");
	return NULL;
}

#define MAX_LISTEN_SOCKETS	16
int listen_socks[MAX_LISTEN_SOCKETS];
int num_listen_socks = 0;



int Gui(void)
{
  int sock = -1;
  int maxfd = 0;
  maxfd = 0;
  fd_set *fdset;
  fdset = NULL;
  int i = 0;
  int ret;
  int newsock;

  struct sockaddr_in from;
  socklen_t fromlen;

  char **addresses;
  addresses = crv_cut(options.gui_addr, " ");


  /* ignore SIGPIPE */
  signal (SIGPIPE, SIG_IGN);

  for (i = 0; addresses[i]; i++)
  {
	/* Prepare TCP socket for receiving connections */
	sock = crv_server_listen( options.gui_port, 5, addresses[i], AF_INET, 1);
	if (sock == -1) {
	  fprintf( stderr, "%s\n", "main(): server_listent() failed");
	  return (-1);
	}

	/* Add comment (listening on several adress )*/
	listen_socks[num_listen_socks] = sock;
	num_listen_socks++;
  }
  

  signal(SIGCHLD, crv_sigchld_handler);


  for (i = 0; i < num_listen_socks; i++)
  	if (listen_socks[i] > maxfd)
  	  maxfd = listen_socks[i];
  //crv_daemon_mode();
  fprintf(stderr, "#\n# GUI\n#\n# Vous pouvez vous connectez sur l'interface web\n en entrant dans votre navigateur internet l'adresse suivante:\n http://%s:%d\n", options.gui_addr, options.gui_port);
  for (;;)
  {
	if (fdset != NULL)
	  crv_free(fdset);
	newsock = -1;
	int pid;
	
	fdset = (fd_set *) calloc(howmany(maxfd + 1, NFDBITS), sizeof(fd_mask));

	for ( i = 0; i < num_listen_socks; i++) {
	  FD_SET(listen_socks[i], fdset);
	}

	/* wait in select until there is a connection. */
	ret = select(maxfd+1, fdset, NULL, NULL, NULL);
	if (ret < 0 && errno != EINTR)  
	  fprintf( stderr, "select: %.100s\n", strerror(errno));
	
	for ( i = 0; i < num_listen_socks; i++)
	  if ( FD_ISSET(listen_socks[i], fdset)) {
		fromlen = sizeof(from);
		newsock = accept(listen_socks[i], (struct sockaddr *)&from, &fromlen);
		if (newsock < 0) {
		  if (errno != EINTR && errno != EWOULDBLOCK )
			fprintf(stderr, "accept: %.100s\n", strerror(errno));
			continue;
		  }
	  	
		if (crv_unset_nonblock(newsock) == -1) {
		  fprintf ( stderr, "Unset nonblock failed");
		  continue;
		}
	
	switch((pid = fork()))
	{
	  // fork() Error 
	  case -1:
		exit (EXIT_FAILURE);
	  
	  // First child process 
	  case 0:
		close(sock);
		char *file = NULL;
		file = handle_connection(newsock);
		shutdown (newsock, 2);
		if (file ) {
			if(options.debug == 1)
				fprintf(stderr, "Fichier a transferer '%s'\n", file);
			handle_get (0, file);
			crv_free (file);
		}
		close(newsock);
		exit(EXIT_SUCCESS);
	  default:
		close(newsock);
	}
	
	} /* End  of FD_ISSET */
  } //End of for(;;) loop
  
  return (0);
}
