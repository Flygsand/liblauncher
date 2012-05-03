bin_PROGRAMS = lcontrol

lcontrol_SOURCES = lcontrol.cc launcher.cc
noinst_HEADERS = lcontrol.h

LDADD = $(top_builddir)/src/lib/liblauncher.a

AM_CPPFLAGS = $(WX_CPPFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS_ONLY) -Wno-long-long
AM_CFLAGS = $(WX_CFLAGS_ONLY)
LIBS = $(WX_LIBS)
