include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/ntfy
plugin_LTLIBRARIES = libntfy.la
plugin_DATA = ntfy.png

libntfy_la_CFLAGS = @EINA_CFLAGS@ @NOTIFY_CFLAGS@
libntfy_la_LDFLAGS = @EINA_LIBS@ @NOTIFY_LIBS@ -module -avoid-version

EXTRA_DIST += $(plugin_DATA) \
	ntfy.svg

libntfy_la_SOURCES = ntfy.c ntfy.h

