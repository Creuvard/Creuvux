/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef GET_H
#define GET_H

extern int Get(SSL *, const char *, off_t, off_t, const char *);
extern int pre_Get(char **);

#endif
