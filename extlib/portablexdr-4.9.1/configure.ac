AC_INIT(portablexdr, 4.9.1)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_PROG_LEX
AC_PROG_YACC

AC_CHECK_PROGS(AR, ar)

dnl Look for an external 'cpp' program which can run on a file with any
dnl extension.  The normal CPP can only run on files with a '.c'
dnl extension, therefore we prefer to use /usr/bin/cpp if it exists.
AC_PATH_PROG([EXTCPP], [cpp], [no])
if test "x$EXTCPP" = "xno"; then
  AC_MSG_FAILURE([Cannot find a working 'cpp' (C preprocessor) program])
fi
AC_DEFINE_UNQUOTED([EXTCPP], ["$EXTCPP"],
	[The C preprocessor command (may include command line options).])

MINGW_EXTRA_LDFLAGS=
case "$host" in
 *-*-mingw*)
   MINGW_EXTRA_LDFLAGS="-no-undefined"
   ;;
esac
AC_SUBST(MINGW_EXTRA_LDFLAGS)

AC_CHECK_HEADERS([arpa/inet.h sys/param.h])
AC_CHECK_FUNCS([ntohl htonl ntohs htons])

AC_OUTPUT(Makefile)

