AC_DEFUN([ML_DEBUG], [dnl
	AC_MSG_CHECKING([if debugging is enabled])
	AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debug code]))
	
	if test x$enable_debug = x;
	then
		enable_debug=no
	fi
	
	if test x$enable_debug = xyes;
	then
		CFLAGS="$CFLAGS -pedantic -Wall"
		CXXFLAGS="$CXXFLAGS -pedantic -Wall"
	else
		AC_DEFINE(NDEBUG, 1, [Define this to skip debugging code.])
	fi
	
	AC_MSG_RESULT([$enable_debug])dnl
])
