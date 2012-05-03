AC_PREREQ(2.59)
AC_INIT(Launcher, 1.0, [tommie@gannert.se])
AC_CONFIG_AUX_DIR(autotools)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR(src/lib/launcher.c)
AM_CONFIG_HEADER(src/include/config.h)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_STDC_HEADERS

AM_OPTIONS_WXCONFIG
AM_PATH_WXCONFIG(2.6.0, wxWin=yes, wxWin=no)
AM_CONDITIONAL(HAVE_WXWIDGETS, test x$wxWin = xyes)

ML_DEBUG

AC_OUTPUT([
	Makefile
	src/Makefile
	src/console/Makefile
	src/control/Makefile
	src/include/Makefile
	src/lib/Makefile
	src/tests/Makefile])
