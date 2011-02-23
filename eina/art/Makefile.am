include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/art
plugin_LTLIBRARIES = libart.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/art
include_HEADERS = \
	eina-art.h         \
	eina-art-backend.h \
	eina-art-search.h  \
	art.h

libart_la_CFLAGS  = @EINA_CFLAGS@ -DART_DEBUG=0
libart_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libart_la_SOURCES = \
	$(include_HEADERS)       \
	eina-art.c               \
	eina-art-backend.c       \
	eina-art-search.c        \
	eina-art-test-backends.h \
	eina-art-test-backends.c \
	art.c

plugin_DATA = art.ini
EXTRA_DIST += art.ini


-include $(top_srcdir)/git.mk
