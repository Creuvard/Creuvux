/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>
#include <netdb.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>


#include "creuvard.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/* Strdup() like */
char *crv_strdup (const char * s)
{
  char *copy = strdup (s);

  /* Quit program if memory allocation failed */
  if (copy == NULL)
	abort();
  else
	return copy;
}

/* Malloc() like */
void *crv_malloc (size_t size)
{
  void *ptr;
  if (size == 0) {
	fprintf(stderr, "%s\n", "crv_malloc: zero size ");
	abort();
  }
  
  ptr = malloc(size);
  if (ptr == NULL)
	abort();
  return ptr;
}

/* Free() like */
void
crv_free(void *ptr)
{
	if (ptr == NULL) {
	  fprintf(stderr, "%s\n", "crv_free: NULL pointer given as argument");
	  abort();
	}
	free(ptr);
}

/* Calloc like */
void *
crv_calloc(size_t nmemb, size_t size)
{
    void *ptr;

    if (size == 0 || nmemb == 0)
        abort();
    if (SIZE_MAX / nmemb < size)
        abort();
    ptr = calloc(nmemb, size);
    if (ptr == NULL)
        abort();
    return ptr;
}

/* Realloc like */
/*
void *
crv_realloc(void *ptr, size_t nmemb, size_t size)
{
    void *new_ptr;
    size_t new_size = nmemb * size;

    if (new_size == 0)
        abort();
    if (SIZE_T_MAX / nmemb < size)
        abort();
    if (ptr == NULL)
        new_ptr = malloc(new_size);
    else
        new_ptr = realloc(ptr, new_size);
    if (new_ptr == NULL)
        abort();
    return new_ptr;
}
*/

/* Ensure that standart files descriptor 0, 1 and 2 are open or directed to /dev/null */
void
crv_sanitise_stdfd(void)
{
	int nullfd, dupfd;

	if ((nullfd = dupfd = open("/dev/null"/*_PATH_DEVNULL*/, O_RDWR)) == -1) {
		fprintf(stderr, "Couldn't open /dev/null: %s", strerror(errno));
		exit(1);
	}
	while (++dupfd <= 2) {
		/* Only clobber closed fds */
		if (fcntl(dupfd, F_GETFL, 0) >= 0)
			continue;
		if (dup2(nullfd, dupfd) == -1) {
			fprintf(stderr, "dup2: %s", strerror(errno));
			exit(1);
		}
	}
	if (nullfd > 2)
		close(nullfd);
}


/*
 * Fopen() like, with race condition 
 */
FILE *crv_fopen(const char *file, const char *mode)
{
  FILE *fd = NULL;
  int fp = 0;
  struct stat st;
  int i;
  int flag = 0;
  int flag_plus = 0;
  int flag_bin = 0;
  
  for (i = 0; mode[i]; i++)
  {
	/* mode READ "r" . */
	if ( (mode[i] == 'r') && (i == 0))
	  flag = 1;
	
	/* mode WRITE "w" */
	if ( (mode[i] == 'w') && (i == 0))
	  flag = 2;
	
	/* mode APPEND "a" */
	if ( (mode[i] == 'a') && (i == 0))
	  flag = 3;
	
	/* mode READ|WRITE|APPEND "+" */
	if ( (mode[i] == '+') && (i == 1))
	  flag_plus = 1;
	
	/* mode READ|WRITE|APPEND "b" */
	if ( (mode[i] == 'b') && (i == 1))
	  flag_bin = 1;
  }
  
  if (flag == 0)
  {
	fprintf(stderr, "%s\n", "crv_fopen(): Err[001] check mode");
	return (NULL);
  }

	if (flag == 1) /* READ */
	{
	  if (flag_plus == 1)
		fp = open (file, O_RDONLY | O_WRONLY, 0);
	  else if (flag_plus == 0) 
		fp = open (file, O_RDONLY , 0);
	}

	if (flag == 2) /* WRITE */
	{
		if (flag_plus == 1)
		  fp = open (file, O_RDONLY | O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);
		else if (flag_plus == 0)
		  fp = open (file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	}

	if (flag == 3) /* APPEND */
	{
	  if (flag_plus == 1)
		fp = open (file, O_RDONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
	  else if (flag_plus == 0)
		fp = open (file, O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
	}

  if (fp < 0) {
	fprintf( stderr, "%s%s%s%s\n", "crv_fopen(): Err[002] open(", file, ") failed. Err: ", strerror(errno));
	return (NULL);
  }

  if (fstat (fp, &st) != 0) {
	close(fp);
	fprintf( stderr, "%s%s%s%s\n", "crv_fopen(): Err[003] stat(", file, ") failed. Err: ", strerror(errno));
	return (NULL);
  }
  
  if (!S_ISREG (st.st_mode)) {
	  close(fp);
	  fprintf(stderr, "%s%s%s\n", "crv_fopen(): Err[004] ", file, " is not a regular file");
      return (NULL);
  } 

  fd = fdopen (fp, mode);

  if (fd == NULL) {
	fprintf(stderr, "%s%s%s%s%s%s\n", "crv_fopen(): Err[005] fdopen(", file, ", \"", mode,"\") ->  fdopen(). Err: ", strerror(errno));
    fclose (fd);
    return (NULL);
  }
  
  return fd;
}

/*
 * cut string in several short string in fonction of separator
 */
char **
crv_cut (const char *str, const char *separator)
{
  char **a = NULL;
  int a_size = 0;
  int a_len = 0;
  int i;
  int separator_len = strlen (separator);
  const char *p0;
  char *p1;
/* Initialisations */
  if (NULL == (a = malloc (SIZE * sizeof *a)))
    return (NULL);
  a_size = SIZE;
  a_len = 0;
  p0 = str;
/* Recupration des sous-chanes */
  while (p0 && (p1 = strstr (p0, separator)))
    {
      if (a_len >= (a_size + 2))
	{
/* L'espace initialement prvu n'est pas suffisant :nous agrandissons le tableau */
	  a_size += SIZE;
	  if (NULL == (a = realloc (a, a_size * sizeof *a)))
	    return (NULL);
	}
      if (NULL == (a[a_len] = calloc ((size_t) (p1 - p0 + 1), sizeof **a)))
	{
	  for (i = 0; i < a_len; i++)
	    free (a[i]);
	  return (NULL);
	}
      memcpy (a[a_len], p0, (size_t) (p1 - p0));
      p0 = p1 + separator_len;
      a_len++;
    }
/* Nous recuprons la dernire chane */
  if (NULL == (a[a_len] = strdup (p0)))
    {
      for (i = 0; i < a_len; i++)
	free (a[i]);
      free (a);
      return (NULL);
    }
  a_len++;
  a[a_len] = NULL;
  return (a);
}

/*
 * chdir() like. Fonction use fd
 */
int
crv_chdir (const char *path)
{
  int fd;
  int result = -1;
  
  if (strlen(path) == 0)
	fd = open (".", O_RDONLY, 0);
  else
	fd = open (path, O_RDONLY, 0);
  if (fd < 0) {
	fprintf( stderr, "%s%s%s%s\n", "crv_chdir(): Err[001] open(", path, ") failed with error : ", strerror(errno));
	return (-1);
  }
  
  result = fchdir (fd);
  if (result == -1) {
	close(fd);
	fprintf( stderr, "%s%s\n", "crv_chdir(): Err[002] fchdir(fd) failed with error : ", strerror(errno));
	return (-1);
  }

  close(fd);
  return (0);
}


/*
 * Drop privilege temporaly
 */
static uid_t priv_uid;

int crv_drop_priv_temp (const char *username)
{
  struct passwd *pw;
  uid_t old_euid = geteuid();

  if ( !(pw = getpwnam(username))) {
	fprintf(stderr, "%s\%s%sn", "drop_priv_temp(): Err[001] getpwnam(", username, ") failed");
	return (-1);
  }
  
  if ( setreuid(getuid(), old_euid) < 0) {
	fprintf( stderr, "%s%s\n", "drop_priv_temp(): Err[002] setreuid() failed with error -> ", strerror(errno));
	return (-1);
  }

  /* Set effective UID */
  if (seteuid(pw->pw_uid) < 0) {
	fprintf( stderr, "%s\n", "drop_priv_temp(): Err[003] seteuid() failed ");
	return (-1);
  }

  /* Get effective UID */
  if (geteuid() != pw->pw_uid) {
	fprintf( stderr, "%s\n", "drop_priv_temp(): Err[004] geteuid() != pw->pw_uid ");
	return (-1);
  }
  
  endpwent();
  priv_uid = old_euid;
  return (0);
}

/*
 * Restore privilege
 */

int crv_restore_priv(void)
{
  int error = 0;

  /* Set old UID */
  if (setuid(priv_uid) < 0)
	error = -1;

  /* check if were successful */
  if (geteuid() != priv_uid)
	error = -1;

  return error;
}

/*
 * Give size of file
 */
off_t
crv_filesize (const char *file)
{
  int fd = 0;
  struct stat st;
  fd = open (file, O_RDONLY, 0);
  if (fd < 0) {
	fprintf( stderr, "%s%s%s%s\n", "crv_filesize(): Err[001] open(", file, ",  O_RDONLY, 0) failed with error -> ", strerror(errno));
	return (-1);
  }
  
  if (fstat (fd, &st) != 0) {
	close(fd);
	fprintf( stderr, "%s%s%s%s\n", "crv_filesize(): Err[002] stat(", file, ") failed with error -> ", strerror(errno));
	return (-1);
  }

  if (!S_ISREG (st.st_mode)) {
      fprintf (stderr, "%s%s%s\n", "crv_filesize(): Err[003] \"", file, "\" is not regular file");
      close (fd);
      return (-1);
    }
  close (fd);
  return st.st_size;
}

/*
 * determine file type (work with extention)
 */

char *crv_file_type( const char *file)
{
  const char *vid[] = { ".avi", ".mpg", ".mp4", ".wmv", ".mkv", ".vob", ".mpeg", ".mov", ".ogm" }; /* nb=9 */
  const char *music[] = { ".au", ".mp3", ".ogg", ".wav", ".cda", ".mid", ".m3u", ".wma" }; /* nb=8 */
  const char *picture[] = { ".bmp", ".gif", ".jpeg", ".png", ".tiff", ".jpg", ".eps" }; /* nb=7 */
  const char *arch[] = { ".bz2", ".rar", ".zip", ".jar", ".gz" }; /* nb=5 */
  int c;
  char *p;
  int result = 0;
  char *style = NULL; 
  p = strrchr (file, '.');
  
  /* On regarde l'extention du fichier et on lui attribue une categorie */
  if (p == (char *) NULL) {
    style = crv_strdup("OTHER");
	return (style);
  }
  else
    {

      for (c = 0;(result == 0) && c < 9; c++)
	  {
		if (!strcmp (vid[c], p)) 
		{
		  free(style);
		  style = crv_strdup("VIDEO");
		  return (style);
	    }
	  }
      for (c = 0;(result == 0) && c < 8; c++)
	  {
		if (!strcmp (music[c], p))
	    {
		  free(style);
		  style = crv_strdup("MUSIC");
		  return (style);
	    }
	  }
      for (c = 0;(result == 0) && c < 7; c++)
	  {
		if (!strcmp (picture[c], p))
	    {
		  free(style);
		  style = crv_strdup("PICTURE");
		  return (style);
		}
	  }
      for (c = 0;(result == 0) && c < 5; c++)
	  {
		if (!strcmp (arch[c], p))
	    {
		  free(style);
		  style = crv_strdup("ARCHIVE");
		  return (style);
	    }
	  }
	  free(style);
	  style = crv_strdup("OTHER");
	  return (style);
	}
}

/*
 * Create server
 */

int crv_server_listen(int port, int nb_pers, const char *addr, int addr_family, int block)
{
	int socket_id;
	int optval = 1;
	struct sockaddr_in sockname;

	/* socket creation */
	if ( (socket_id = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "%s\n", "server_listen(): Err[001] socket() failed");
		exit(EXIT_FAILURE);
	}

	/* Set nonblock socket */
	if (block == 1) {
	  if (crv_set_nonblock (socket_id) == -1) {
		close(socket_id);
	  }
	}
	
	/* enables local address reuse */
	if (setsockopt (socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval)) == -1)
	{
		fprintf(stderr, "%s\n", "server_listen(): Err[002] setsockopt() failed");
		exit(EXIT_FAILURE);
	}

	/* make address */
	memset ((char *) &sockname, '\0', sizeof (sockname));
	sockname.sin_family = addr_family;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = inet_addr ( addr );

	/* assigns a name to an unnamed socket */
	if ( (bind (socket_id, (struct sockaddr *) &sockname, sizeof (struct sockaddr_in)) ) == -1)
	{
		fprintf(stderr, "%s%s\n", "server_listen(): Err[003] bind() failed with error -> ", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/* listen socket */
	if ( (listen (socket_id, nb_pers)) == -1)
	{
		fprintf(stderr, "%s\n", "server_listen(): Err[004] listen() failed");
		exit(EXIT_FAILURE);
	}

	return (socket_id);
}


/*
 * Set nonblock
 */
int
crv_set_nonblock(int fd)
{
    int val;

    val = fcntl(fd, F_GETFL, 0);
    if (val < 0) {
        fprintf(stderr, "crv_set_nonblock(): Err[001] fcntl(%d, F_GETFL, 0): %s", fd, strerror(errno));
        return (-1);
    }
    if (val & O_NONBLOCK) {
        fprintf(stderr, "crv_set_nonblock(): Err[002] fd %d is O_NONBLOCK", fd);
        return (0);
    }
    val |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, val) == -1) {
        fprintf(stderr, "crv_set_nonblock(): Err[003] fcntl(%d, F_SETFL, O_NONBLOCK): %s", fd,
            strerror(errno));
        return (-1);
    }
    return (0);
}

/*
 * Unset nonblock
 */
int
crv_unset_nonblock(int fd)
{
    int val;

    val = fcntl(fd, F_GETFL, 0);
    if (val < 0) {
        fprintf(stderr, "crv_unset_nonblock(): Err[001] fcntl(%d, F_GETFL, 0): %s", fd, strerror(errno));
        return (-1);
    }
    if (!(val & O_NONBLOCK)) {
        fprintf(stderr, "crv_unset_nonblock(): Err[002] fd %d is not O_NONBLOCK", fd);
        return (0);
    }
    val &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, val) == -1) {
        fprintf(stderr, "crv_unset_nonblock(): Err[003] fcntl(%d, F_SETFL, ~O_NONBLOCK): %s",
            fd, strerror(errno));
        return (-1);
    }
    return (0);
}


/*
 * Mode daemon
 */
void
crv_daemon_mode (void)
{

  int pid;
  
  pid = fork ();
  if (pid < 0) {
    fprintf( stderr, "%s%s\n", "crv_daemon_mode(): Err[001] fork() failed with error -> ", strerror(errno));
	exit(EXIT_FAILURE);  
  }
  if (pid > 0)
    exit (EXIT_SUCCESS);

  /* Create new session */
  setsid ();

  /* Terminal separation*/
  pid = fork ();
  if (pid < 0) {
	fprintf( stderr, "%s%s\n", "crv_daemon_mode(): Err[001] fork() failed with error -> ", strerror(errno));
	exit(EXIT_FAILURE);
  }
  if (pid > 0)
    exit (EXIT_SUCCESS);

  /* close file descriptors */
  /*
  for (i = getdtablesize (); i >= 0; --i)
    close (i);
  i = open ("/dev/null", O_RDWR);
  dup(i);
  dup(i);
  */
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);

  /* Mise a jour du masque de creation de fichiers par defaut */
  umask (022);
}


/*
 * This is called whenever a child dies. This will then reap any zombies left by exited children.
 */

void
crv_sigchld_handler( int sig)
{
  int save_errno = errno;
  pid_t pid;
  int status;

  while ( (pid = waitpid(-1, &status, WNOHANG) ) > 0 || (pid < 0 && errno == EINTR));
  signal (SIGCHLD, crv_sigchld_handler);
  errno = save_errno;
}


/*
 * chroot() like.
 */
int crv_chroot ( const char *directory)
{
  if (chroot (directory) != 0 || chdir ("/") != 0) { 
	fprintf( stderr, "%s%s%s%s\n", 
	  "crv_chroot(): Err[001] chroot(",
	  directory,
	  ") failed with error -> ",
	  strerror(errno)); 
	return (-1);  
  }
  return (0);
}

/*
 * Drop privilege and chroot in directory
 */
int crv_drop_priv_perm_and_chroot(const char *username, const char *directory)
{
  struct passwd *pw;
  int error = 0;
  uid_t old_uid = getuid();
  gid_t old_gid = getgid();

  if (directory != NULL) {
	if ( !(pw = getpwnam(username)) ) {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [000] ");
	return (-1);
	}
  }
 
  if (directory) 
	error = crv_chroot (directory);
  else {
	if ( !(pw = getpwnam(username)) ) {
	  fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [001] ");
	  return (-1);
	}
	error = crv_chroot(pw->pw_dir);
  }

  if (error == -1) {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [002] ");
	return (-1);
  }
 
  /* Set new eGID */
  if (setegid(pw->pw_gid) != 0) {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [003] ");
	return (-1);
  }
  
  /* Set new GID */
  if (setgid(pw->pw_gid) != 0) {
  	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [004] ");
	return (-1);
  }

  /* Set new eUID */
  if (seteuid(pw->pw_uid) != 0) {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [005] ");
	return (-1);
  }
  
  /* Set new UID */
  if (setuid(pw->pw_uid) != 0) {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [006] ");
	return (-1);
  }
  
  /* verify that the changes were successful */
  if (pw->pw_gid != old_gid && (setegid(old_gid) != -1 || getegid() != pw->pw_gid))
  {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [007] ");
	return (-1);
  }
  if (pw->pw_uid != old_uid && (seteuid(old_uid) != -1 || geteuid() != pw->pw_uid))
  {
	fprintf (stderr, "%s\n", "crv_drop_priv_perm_and_chroot(): Err [008] ");
	return (-1);
  }
  
  return (0);
}

/*
 * strncpy() like from OPENBSD
 */
size_t
crv_strncpy(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;

        /* Copy as many bytes as will fit */
        if (n != 0) {
                while (--n != 0) {
                        if ((*d++ = *s++) == '\0')
                                break;
                }
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (siz != 0)
                        *d = '\0';                /* NUL-terminate dst */
                while (*s++)
                        ;
        }

        return(s - src - 1);        /* count does not include NUL */
}



/*
 * strncat() like from OPENBSD
 */
size_t
crv_strncat(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;
        size_t dlen;

        /* Find the end of dst and adjust bytes left but don't go past end */
        while (n-- != 0 && *d != '\0')
                d++;
        dlen = d - dst;
        n = siz - dlen;

        if (n == 0)
                return(dlen + strlen(s));
        while (*s != '\0') {
                if (n != 1) {
                        *d++ = *s;
                        n--;
                }
                s++;
        }
        *d = '\0';

        return(dlen + (s - src));        /* count does not include NUL */
}

/*
 * crv_du(): displays the file system block usage for file.
 */
off_t
crv_du (const char *file)
{
  int fd = 0;
  struct stat st;
  fd = open (file, O_RDONLY, 0);
  if (fd < 0) {
    close (fd);
    fprintf (stderr, 
			  "%s%s\n", 
			  "crv_du(): Err[001] open() failed with error -> ",
			  strerror(errno));
      return (-1);
  }

  if (fstat (fd, &st) != 0)  {
    close (fd);
    fprintf (stderr, 
			  "%s%s\n", 
			  "crv_du(): Err[002] fstat() failed with error -> ",
			  strerror(errno));
	return (-1);
  }

  if (!S_ISREG (st.st_mode))  {
    close (fd);
    fprintf (stderr, 
			  "%s%s%s\n", 
			  "crv_du(): Err[003] ", file, "is not regular file");
	return (-1);
  }
  close (fd);
  return st.st_size;
}


/*
 * getenv() like.
 */
char *crv_getenv(const char *env)
{
char *value = getenv (env);
  if (!value)
    {
      fprintf (stderr, "%s%s%s\n", "Environnement '", env,
           "' non initialisé ");
      exit (EXIT_FAILURE);
    }
  else
    return value;
}

/*
 * Create client.
 */
int crv_client_create(int port, const char *addr, int addr_family)
{
  struct sockaddr_in sockname;
  int optval;
  int socket_id;
  struct hostent *host_address;

  /* Give Internet host referenced by name or by address */
  if ( NULL == (host_address = gethostbyname(addr))) {
	fprintf( stderr, "%s%s%s\n", "crv_server_listen(): Err[001] gethostbyname(", addr, ") failed");
	return (-1);
  }

  /* socket creation */
  if ( (socket_id = socket( addr_family, SOCK_STREAM, 0)) == -1) {
	fprintf( stderr, "%s\n", "crv_client_create(): Err[002] socket() failed");
	return (-1);
  }

  optval = 1;
  setsockopt (socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

  /* Connection */
  sockname.sin_family = addr_family;
  sockname.sin_port = htons(port);
  //sockname.sin_addr.s_addr = inet_addr (addr);
  memcpy((char *) &(sockname.sin_addr.s_addr), host_address->h_addr, (size_t)host_address->h_length);
  
  if ( connect(socket_id, (struct sockaddr *) &sockname, sizeof(struct sockaddr_in)) == -1)
  {
	fprintf( stderr, "%s%s\n", "crv_client_create(): Err[003] Can't connect socket to ", addr);
	return (-1);
  }

  return (socket_id);
}

#ifndef WIN32
#else
void
couleur (int couleurDuTexte, int couleurDeFond)
{
  HANDLE H = GetStdHandle (STD_OUTPUT_HANDLE);
  SetConsoleTextAttribute (H, couleurDeFond * 16 + couleurDuTexte);
}
#endif


/*
 * Remove all file in drirectory (not recursive)
 */
int
crv_remove_files_in_directory (char *dirname)
{
  DIR *dirp;
  struct dirent *f;
  struct stat buf;
  char *cwd = NULL;
  int result = -1;
  
  cwd = getcwd (cwd, SIZE);
  result = crv_chdir (dirname);
  if (result == -1) {
	fprintf( stderr, "%s\n", "crv_remove_files_in_directory(): Err[001] crv_chdir() failed");
	return (-1);
  }
  dirp = opendir (".");
  if (dirp)
  {
	while ((f = readdir (dirp))!= NULL)
	{
	  if (stat (f->d_name, &buf))
	  {
		fprintf( stderr, 
			"%s%s%s%s\n",  
			"crv_remove_files_in_directory(): Err[001] stat(",
			f->d_name,
			") failed with error -> ", 
			strerror(errno));
		closedir (dirp);
		return (-1);
	  }
	  unlink (f->d_name);
	}
  }
  closedir (dirp);
  
  result = crv_chdir (cwd);
  if (result == -1) {
	fprintf( stderr, "%s\n", "crv_remove_files_in_directory(): Err[002] crv_chdir() failed");
	return (-1);
  }
  return (0);
}


/*
 * Sha1 from OpenBSD src.
 */
#include <openssl/sha.h>
#define BUFSIZE	1024*16

char *
crv_sha1(const char *filename)
{
	FILE *Fd = NULL;
	char *sh = NULL;
	char ha[SIZE];
	sh = crv_malloc(SHA_DIGEST_LENGTH);
	
	Fd = crv_fopen(filename, "r");
	if (Fd == NULL) {
		fprintf(stderr, "%s\n", "crv_sha1(): Err[001] crv_fopen() failed");
		return (NULL);
	}
	SHA_CTX c;
	unsigned char md[SHA_DIGEST_LENGTH];
	int fd;
	int i;
	unsigned char buf[BUFSIZE];

	fd=fileno(Fd);
	SHA1_Init(&c);
	for (;;)
	{
		i=read(fd,buf,BUFSIZE);
		if (i <= 0) break;
		SHA1_Update(&c,buf,(unsigned long)i);
	}
	SHA1_Final(&(md[0]),&c);

	for (i=0; i<SHA_DIGEST_LENGTH; i++) {
		snprintf(sh, SHA_DIGEST_LENGTH, "%02x", md[i]);
		if (i==0)
			crv_strncpy(ha, sh, sizeof(ha));
		else 
			crv_strncat(ha, sh, sizeof(ha));
	}
	memset(sh, 0, sizeof(sh));
	sh = crv_strdup(ha);
	memset(ha, 0, sizeof(ha));
	fclose(Fd);
	return (sh);	
}
/*
void crv_sha1(const char *filename, FILE *d)
{
	FILE *Fd = NULL;
	Fd = crv_fopen(filename, "r");
	if (Fd == NULL) {
		fprintf(stderr, "%s\n", "crv_sha1(): Err[001] crv_fopen() failed");
		return ;
	}
	SHA_CTX c;
	unsigned char md[SHA_DIGEST_LENGTH];
	int fd;
	int i;
	unsigned char buf[BUFSIZE];

	fd=fileno(Fd);
	SHA1_Init(&c);
	for (;;)
	{
		i=read(fd,buf,BUFSIZE);
		if (i <= 0) break;
		SHA1_Update(&c,buf,(unsigned long)i);
	}
	SHA1_Final(&(md[0]),&c);

	for (i=0; i<SHA_DIGEST_LENGTH; i++)
		fprintf(d, "%02x",md[i]);
	fclose(Fd);
	return;	
}
*/


/*
 * strncmp() like.
 */
int crv_strncmp(const char *s1, const char *s2)
{
  size_t len_s1 = 0;
  size_t len_s2 = 0;
  int result = -1;
  if (s1 == NULL) {
	fprintf(stderr, "%s", "crv_strncmp(): Err[001] first value is NULL\n");
	return (-1);
  }
  if (s2 == NULL) {
	fprintf(stderr, "%s", "crv_strncmp(): Err[002] second value is NULL\n");
	return (-1);
  }

  len_s1 = strlen(s1);
  len_s2 = strlen(s2);
  if ( len_s1 > len_s2)
	result = strncmp( s1, s2, len_s1);
  else
	result = strncmp( s1, s2, len_s2);
  return (result);
}


/*
 * strcasestr() like
 * Find the first occurrence of find in s, ignore case.
 * From OpenBSD src
 *
 */
char *
crv_strcasestr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = (char)tolower((unsigned char)c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0)
          return (NULL);
      } while ((char)tolower((unsigned char)sc) != c);
    } while (strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

/*
 * crv_move()
 * Like shell "mv" command.
 *
 */
int crv_move(const char *old_file, const char *new_file)
{
	FILE *old_fd, *new_fd;
	char buf[BUFSIZE];
	old_fd = crv_fopen(old_file, "r");
	if (old_fd == NULL) {
		fprintf(stderr, "%s%s", "crv_move(): Err[001] Can't open ", old_file); 
		return (-1);
	}

	new_fd = crv_fopen(new_file, "w");
	if (new_fd == NULL) {
		fclose(old_fd);
		fprintf(stderr, "%s%s", "crv_move(): Err[002] Can't open ", new_file); 
		return (-1);
	}

	while (!feof(old_fd))
	{
		size_t l;
		l = fread (buf, sizeof(char), sizeof(buf), old_fd);
		if (ferror (old_fd)) {
			fprintf(stderr, "%s\n", "crv_move(): Err[003]: Read old_file failed");
			return (-1);
		}
		fwrite (buf, sizeof(char), l, new_fd);
		if (ferror (new_fd)) {
			fprintf(stderr, "%s\n", "crv_move(): Err[004]: Write new_file failed");
			return (-1);
		}


	}
	return (0);
}

/*
 * Create tmp file filename.XXXX
 */
char *crv_mkstemp(const char *prefix, const char *directory)
{
  char sfn[SIZE];
  FILE *sfp;
  char *tmp_char = NULL;
  int fd;
  off_t len = 0;
  len = strlen (directory) + strlen(prefix) + 12;
  if (len > SIZE) {
	fprintf(stderr, "%s\n", 
	"crv_mkstemp(): /directory/prefix.XXXXXXXXXX is too long");
	return (NULL);
  }

  (void)crv_strncpy(sfn, directory, sizeof(sfn));
  (void)crv_strncat(sfn, prefix, sizeof(sfn));
  (void)crv_strncat(sfn, ".XXXXXXXXXX", sizeof(sfn));

  if ((fd = mkstemp(sfn)) == -1 ||
	  (sfp = fdopen(fd, "w+")) == NULL) 
  {
	if (fd != -1) {
	  unlink(sfn);
	  close(fd);
	}
	return (NULL);
  }
  fclose(sfp);
  tmp_char = crv_strdup(sfn);
  return (tmp_char);
}

