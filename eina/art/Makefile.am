include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/art
plugin_LTLIBRARIES = libart.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/art
include_HEADERS = \
	art.h

libart_la_CFLAGS  = @EINA_CFLAGS@
libart_la_LDFLAGS = @EINA_LIBS@
libart_la_SOURCES = \
	$(include_HEADERS) \
	art.c

plugin_DATA = art.ini
EXTRA_DIST += art.ini

