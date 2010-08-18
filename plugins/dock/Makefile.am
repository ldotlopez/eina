include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/dock
plugin_LTLIBRARIES = libdock.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/plugins/dock
include_HEADERS = \
	eina-dock.h \
	dock.h

libdock_la_CFLAGS  = @EINA_CFLAGS@
libdock_la_LDFLAGS = @EINA_LIBS@
libdock_la_SOURCES =   \
	$(include_HEADERS) \
	eina-dock.c        \
	dock.c

plugin_DATA = dock.ini

EXTRA_DIST += dock.ini
