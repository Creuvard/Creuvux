/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#ifndef NETWORK_H
#define NETWORK_H
#include <openssl/ssl.h>

typedef struct 
{
  char	*name;		/* SYNC, GET, PUT ... */
  char	*version;	/* Client's version */
  off_t	begin;		/* octet whereis file begin */
  off_t	end;		/* octet where file terminate */
  char	*grp;		/* Groups of user */
  int	flag;		/* Allowed to continue */
  char	*addr;		/* Client's address */
  char	*user;		/* Client's username */
  char	*mail;		/* Username mail */
  char	*sha1;		/* sha1 use with GET and PUT command */
  char	*path;		/* Path of file */
  char	*lst_file;	/* Filename username.xsl */
  char	*filename;	/* Filename to download */
  char	*old_sha1;	/* Use for rename sha1 in real filename COMMENt|MSG */
	char	*genre;			/* Use for create about file*/
	char	*comment;		/* Use for create about file*/

} Command_options;

extern void		initialize_command_opt(Command_options *);
extern void	  server_accept_loop( void );
extern char	  *recup_pseudo (SSL *);
extern char	  *recup_email (SSL *);
extern int	  server_listen(int , int , const char *, int , int );

#endif /* NETWORK_H */
