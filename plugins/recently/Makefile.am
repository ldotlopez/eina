include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/recently
plugin_LTLIBRARIES = librecently.la
plugin_DATA = go-back.png dock.ui

EXTRA_DIST += $(plugin_DATA) \
	go-back.png

librecently_la_CFLAGS = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
librecently_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@ -module -avoid-version

librecently_la_SOURCES = recently.c

