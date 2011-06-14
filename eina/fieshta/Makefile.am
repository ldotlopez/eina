include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/fieshta
plugin_LTLIBRARIES = libfieshta.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/fieshta
include_HEADERS = \
	eina-fieshta-behaviour.h \
	eina-fieshta-plugin.h

libfieshta_la_CFLAGS  = @EINA_CFLAGS@
libfieshta_la_LDFLAGS = @EINA_LIBS@ -avoid-version
libfieshta_la_SOURCES = \
	$(include_HEADERS) \
	eina-fieshta-behaviour.c \
	eina-fieshta-plugin.c

plugin_DATA = fieshta.plugin
EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
