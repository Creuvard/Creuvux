/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef LOG_H
#define LOG_H


#define LOG_MESSAGE_LENGTH 256
#define IP_SIZE 16
#define int_error(msg)  do { handle_error(__FILE__, __LINE__, msg); exit(EXIT_FAILURE); } while (0)
void Log ( const char *, int , const char *, ...);

#endif
