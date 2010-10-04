include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/muine
plugin_LTLIBRARIES = libmuine.la
plugin_DATA = muine.ui

libmuine_la_CFLAGS = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
libmuine_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@ -module -avoid-version

EXTRA_DIST += $(plugin_DATA)

libmuine_la_SOURCES =   \
	muine.c

