/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>


#include "log.h"
#include "server_conf.h"
#include "network.h"
#include "creuvard.h"

/* Server configuration options. */
ServerOptions options;

/* Command options. */
Command_options command;

void
Log (const char *log_level, int chroot_flag, const char *fmt, ...)
{
  va_list args;
  char message[LOG_MESSAGE_LENGTH];
  int message_pos = 0;
  char log_date[] = "YYYY/MM/DD HH:MM:SS ";
  char *p;
  char *bufstr;
  int bufint;
  long buflong;
  time_t t;
  struct tm *tm;
  FILE *fd = NULL;
  va_start (args, fmt);
  
  for (p = (char *) fmt; (p[0] != '\0') && (message_pos < LOG_MESSAGE_LENGTH);
       p++)
    {
      switch (p[0])
	{
	case '%':		/* caractère % litéral */
	  switch (p[1])
	    {
	    case '%':		/* entier long %ld ou %li */
	      message[message_pos++] = '%';
	    case 'l':
	      switch (p[2])
		{
		case 'd':
		case 'i':
		  buflong = va_arg (args, long);
		  message_pos +=
		    snprintf (message + message_pos,
			      (size_t) (LOG_MESSAGE_LENGTH - message_pos),
			      "%ld", buflong);
		  break;
		default:
		  message[message_pos++] = '?';
		}
	      p++;
	      break;
	    case 'd':		/* entier long %d ou %i */
	    case 'i':
	      bufint = va_arg (args, int);
	      message_pos +=
		snprintf (message + message_pos,
			  (size_t) (LOG_MESSAGE_LENGTH - message_pos), "%d",
			  bufint);
	      break;
	    case 's':		/* chaîne %s */
	      bufstr = va_arg (args, char *);
	      while ((message_pos < LOG_MESSAGE_LENGTH) && (bufstr[0]))
		{
		  message[message_pos++] = bufstr[0];
		  bufstr++;
		}
	      break;
	    default:
	      message[message_pos++] = '?';
	    }
	  p++;
	  break;
	default:
	  message[message_pos++] = p[0];
	}
    }
  va_end (args);
  message[message_pos] = '\0';
  t = time (NULL);
  tm = localtime (&t);
  strftime (log_date, sizeof (log_date), "%Y/%m/%d %H:%M:%S", tm);

  if (options.debug != 1) 
  {
	/* We try to open /creuvux.log */
	if (chroot_flag == 1) {
	  if ((fd = fopen ("/creuvux.log", "a")) == NULL) {
		  fprintf(stderr, "%s\n", "log.c : fopen() failed");
		  exit(EXIT_FAILURE);
		}
	} else if (chroot_flag == 0) {
	  /* Wee chdir in CREUVUX_USER home directory and add log in creuvux.log file*/
		if ( crv_chdir(options.chroot_directory) == -1) {
		  fprintf(stderr, "%s\n", "log.c : Chroot() failed");
		  exit(EXIT_FAILURE);
		}
		endpwent();
		if ((fd = fopen ("creuvux.log", "a")) == NULL) {
		  fprintf(stderr, "%s\n", "log.c : fopen() failed");
		  exit(EXIT_FAILURE);
		}
	}
  }
  if (!fd) fd = stderr;
  

  if (!crv_strncmp(log_level, "CRASH"))
	fprintf (fd, "%s (%-7s) [%-5ld] %s\n", log_date, log_level, (long) getpid (), message);
  else 
  if ((!crv_strncmp (log_level, "OK")) ) {
	  fprintf (fd, "%s (%-7s) [%10s:%-5s %-5s] %s\n", log_date, log_level,
	  (const char *)command.user,
	  (const char *)command.mail, 
	  (const char *)command.addr,
	  message);
  }
  else {
	fprintf (fd, "%s (%-7s) [%10s:%-5s %-5ld] %s\n", log_date, log_level,
	(const char *)command.user,
	(const char *)command.mail, 
	(long) getpid (), 
	message);
  }

  if (fd == stderr)
    fflush (fd);
  else
    fclose (fd);
}
