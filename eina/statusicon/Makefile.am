include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/statusicon
plugin_LTLIBRARIES = libstatusicon.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/statusicon
include_HEADERS = 

libstatusicon_la_CFLAGS  = @EINA_CFLAGS@
libstatusicon_la_LDFLAGS = @EINA_LIBS@
libstatusicon_la_SOURCES = \
	statusicon.c

plugin_DATA = statusicon.ini

EXTRA_DIST += statusicon.ini
