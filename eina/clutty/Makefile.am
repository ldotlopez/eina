include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/clutty
plugin_LTLIBRARIES = libclutty.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/clutty
include_HEADERS = \
	eina-cover-clutter.h

libclutty_la_CFLAGS  = @EINA_CFLAGS@ @CLUTTER_GTK_CFLAGS@
libclutty_la_LDFLAGS = @EINA_LIBS@ @CLUTTER_GTK_LIBS@ -module -avoid-version
libclutty_la_SOURCES = \
	$(include_HEADERS) \
	eina-cover-clutter.c \
	clutty.c

plugin_DATA = clutty.ini

EXTRA_DIST += \
	$(plugin_DATA)

-include $(top_srcdir)/git.mk
