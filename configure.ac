AC_PREREQ([2.60])
AC_INIT([mirage2iso], [0.3])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2 no-dependencies])

AC_PROG_CC_C99
AC_USE_SYSTEM_EXTENSIONS
AM_PATH_GLIB_2_0([2.6.0],,
	[AC_MSG_ERROR([This package requires >=glib-2.6.0 to compile])])
PKG_CHECK_MODULES([LIBMIRAGE], [libmirage])
AC_CHECK_FUNCS([ftruncate mmap posix_fallocate])

AC_ARG_WITH([libassuan],
	[AS_HELP_STRING([--without-libassuan],
		[Disable use of libassuan])])
AS_IF([test x"$with_libassuan" != x"no"],
	[AM_PATH_LIBASSUAN([2.0.0], [
		AC_DEFINE([HAVE_LIBASSUAN2], [1], [Define if you have libassuan >= 2])
		AC_DEFINE([HAVE_LIBASSUAN], [1], [Define if you have libassuan])
	], [AM_PATH_LIBASSUAN([], [
		AC_DEFINE([HAVE_LIBASSUAN], [1], [Define if you have libassuan])
	])])])

AC_SYS_POSIX_TERMIOS
AS_IF([test x"$ac_cv_sys_posix_termios" = x"yes"],
	[AC_DEFINE([HAVE_TERMIOS], [1], [Define if you have termios headers and functions])])

AC_CONFIG_HEADER([mirage-config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
