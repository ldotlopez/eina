include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/muine
plugin_LTLIBRARIES = libmuine.la
libmuine_la_CFLAGS  = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
libmuine_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
libmuine_la_SOURCES = \
	eina-muine.c        \
	eina-muine.h        \
	eina-muine-plugin.h \
	eina-muine-plugin.c \
	resources.c

plugin_DATA = \
	muine.plugin

eina_gresources_xml  = resources.xml
eina_gresources_name = eina_muine
include $(top_srcdir)/build/Makefile.am.gresources

EXTRA_DIST += \
	$(plugin_DATA)   \
	resources.xml.in \
	main.ui

-include $(top_srcdir)/git.mk
