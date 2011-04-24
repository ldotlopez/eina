include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/fieshta
plugin_LTLIBRARIES = libfieshta.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/fieshta
include_HEADERS = \
	eina-fiestha-behaviour.h \
	fieshta.h

libfieshta_la_CFLAGS  = @EINA_CFLAGS@
libfieshta_la_LDFLAGS = @EINA_LIBS@ -avoid-version
libfieshta_la_SOURCES = \
	$(include_HEADERS) \
	eina-fiestha-behaviour.c \
	fieshta.c

plugin_DATA = fieshta.ini
EXTRA_DIST += fieshta.ini

-include $(top_srcdir)/git.mk
