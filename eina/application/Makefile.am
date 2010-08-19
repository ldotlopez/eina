include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/application
plugin_LTLIBRARIES = libapplication.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/application
include_HEADERS = \
	application.h

libapplication_la_CFLAGS  = @EINA_CFLAGS@
libapplication_la_LDFLAGS = @EINA_LIBS@
libapplication_la_SOURCES = \
	$(include_HEADERS)      \
	application.c

plugin_DATA = application.ini

EXTRA_DIST += application.ini
