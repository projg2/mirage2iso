AC_PREREQ([2.60])
AC_INIT([mirage2iso], [0.4.2])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 foreign no-dependencies parallel-tests subdir-objects])

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
PKG_CHECK_MODULES([GLIB], [glib-2.0])
dnl vv - workaround for libmirage missing reqs - vv
PKG_CHECK_MODULES([LIBMIRAGE], [libmirage >= 2.0.0])
PKG_CHECK_EXISTS([libmirage >= 3.0.0], [AC_DEFINE(HAVE_LIBMIRAGE3, [1], [Define if you have libmirage >= 3])])

dnl libmirage-3.0.6 fixes .isz / .dmg support
PKG_CHECK_EXISTS([libmirage >= 3.0.6], [have_working_isz_dmg=yes], [have_working_isz_dmg=no])
AM_CONDITIONAL([HAVE_WORKING_ISZ_DMG], [test x"$have_working_isz_dmg" = x"yes"])

AC_SYS_LARGEFILE
AC_CHECK_FUNCS([posix_fallocate])

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

AC_CONFIG_HEADER([src/mirage-config.h])
AC_CONFIG_FILES([Makefile tests/Makefile])
AC_OUTPUT
