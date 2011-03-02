include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/mpris
plugin_LTLIBRARIES = libmpris.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/mpris
include_HEADERS = 

libmpris_la_CFLAGS  = @EINA_CFLAGS@
libmpris_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libmpris_la_SOURCES = \
	mpris.h mpris-spec.h \
	eina-mpris-player.c eina-mpris-player.h \
	bridge.c \
	mpris-native.c
# mpris-rb.c 

plugin_DATA = mpris.ini

EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
