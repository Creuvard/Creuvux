#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([creuvux], [0.78], [creuvard@creuvux.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/creuvux.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL


SQL_CFLAGS=[`pkg-config --cflags sqlite3`]
AC_SUBST(SQL_CFLAGS)

SQL_LIBS=[`pkg-config --libs sqlite3`]
AC_SUBST(SQL_LIBS)

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lsqlite3':
AC_CHECK_LIB([sqlite3], [sqlite3_open])
# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
AC_CHECK_LIB([crypto], [X509_get_ext_count])
# FIXME: Replace `main' with a function in `-lssl':
AC_CHECK_LIB([ssl], [SSL_get_error])
AC_CHECK_LIB(pthread, [pthread_create])
AC_CHECK_LIB(z, [deflateInit])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 endpwent fchdir getcwd gethostbyname gettimeofday memset socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
