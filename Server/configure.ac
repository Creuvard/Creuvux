#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([creuvuxd], [0.81], [creuvard@creuvux.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/creuvuxd.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lz':
AC_CHECK_LIB([z], [main])
# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
AC_CHECK_LIB([crypto], [X509_get_ext_count])
# FIXME: Replace `main' with a function in `-lssl':
AC_CHECK_LIB([ssl], [SSL_get_error])
AC_CHECK_LIB(pthread, [pthread_create])
AC_CHECK_LIB(z, [deflateInit])


#SQLite 
PKG_CHECK_MODULES([SQL], [sqlite3])
SQL_CFLAGS=[`pkg-config --cflags sqlite3`]
AC_SUBST(SQL_CFLAGS)

#SQL_LIBS=[`pkg-config --libs sqlite3`]
AC_SUBST(SQL_LIBS)

#MySQL ARGS
MYSQL_CONFIG_PATH=
AC_ARG_WITH(mysql_config_path,
[ AC_HELP_STRING([--with-mysql_config_path],[mysql_config full path (default=)])],[
  if test "$withval" != "no" -a "$withval" != "yes"; then
    MYSQL_CONFIG_PATH=$withval
  fi
])
AC_PATH_PROG([MYSQL_CONFIG], mysql_config,[no], ${MYSQL_CONFIG_PATH}:${PATH})
if test "x$MYSQL_CONFIG" = "xno"; then
  AC_MSG_ERROR([Could not find mysql_config])
fi

MYSQL_CFLAGS=`${MYSQL_CONFIG} --cflags`
MYSQL_LIBS=`${MYSQL_CONFIG} --libs`
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 endpwent fchdir getcwd gettimeofday inet_ntoa memset select socket strcasecmp strchr strcspn strdup strerror strrchr strstr strtol])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/pki/Makefile
                 scripts/Makefile
                 src/Makefile])
AC_OUTPUT
