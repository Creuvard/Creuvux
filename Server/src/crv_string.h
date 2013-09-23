/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef CRV_STRING_H
#define CRV_STRING_H

#include <stdio.h>
typedef struct
{
  char *str;                    /* chaîne */
  int size;                     /* Taille de l'espace alloué */
  int len;                      /* Longueur de la chaîne */
} string_t;

extern string_t * string_new (void);
extern string_t * string_new_initial_len (void);
extern void string_free (string_t *);
extern void string_ajout (string_t *, const char *);

#endif /* CRV_STRING_H */

