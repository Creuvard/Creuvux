/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef CREUVARD_H
#define CREUVARD_H

#include <stdio.h>

#define SIZE 1024

extern char	  	*crv_strdup (const char * );
extern void	  	*crv_malloc (size_t );
extern void			*crv_calloc(size_t , size_t );
/*extern void	  *crv_realloc(void *, size_t , size_t );*/
extern void	  	crv_free(void *);
extern void	  	crv_sanitise_stdfd(void);
extern FILE	  	*crv_fopen(const char *, const char *);
extern char	  	**crv_cut (const char *, const char *);
extern int	  	crv_chdir (const char *);
extern int	  	crv_drop_priv_temp (const char *);
extern int	  	crv_restore_priv(void);
extern off_t  	crv_filesize (const char *);
extern char	  	*crv_file_type( const char *);
extern int	  	crv_server_listen(int , int , const char *, int , int );
extern int	  	crv_set_nonblock(int );
extern int	  	crv_unset_nonblock(int );
extern void	  	crv_daemon_mode (void);
extern void	  	crv_sigchld_handler( int );
extern int	  	crv_drop_priv_perm_and_chroot(const char *, const char *);
extern int	  	crv_chroot ( const char *);
extern size_t 	crv_strncpy(char *, const char *, size_t );
extern size_t 	crv_strncat(char *, const char *, size_t );
extern off_t  	crv_du (const char *);
extern char			*crv_sha1(const char *);
extern char			*crv_mkstemp(const char *, const char *);
extern int			crv_strncmp(const char *, const char *);
extern char			*crv_real_name(const char *);
extern char			*crv_last_dirname(const char *);


#endif /* CREUVARD_H */
