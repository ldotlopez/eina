include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/muine
plugin_LTLIBRARIES = libmuine.la
libmuine_la_CFLAGS  = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
libmuine_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
libmuine_la_SOURCES = \
	eina-muine.c    \
	eina-muine.h    \
	eina-muine-ui.h \
	muine.h         \
	muine.c

plugin_DATA = \
	muine.ini

eina_ui2h_ui = eina-muine.ui
eina_ui2h_name = __eina_muine_ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

EXTRA_DIST += $(plugin_DATA)


-include $(top_srcdir)/git.mk
