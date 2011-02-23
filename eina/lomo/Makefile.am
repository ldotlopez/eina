include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/lomo
plugin_LTLIBRARIES = liblomo.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/lomo
include_HEADERS = \
	lomo.h

liblomo_la_CFLAGS  = @EINA_CFLAGS@
liblomo_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
liblomo_la_SOURCES =   \
	$(include_HEADERS) \
	lomo.c

plugin_DATA = lomo.ini

EXTRA_DIST += lomo.ini

-include $(top_srcdir)/git.mk
