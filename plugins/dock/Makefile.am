include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/dock
plugin_LTLIBRARIES = libdock.la
libdock_la_CFLAGS  = @EINA_CFLAGS@
libdock_la_LDFLAGS = @EINA_LIBS@
libdock_la_SOURCES = \
	eina-dock.c \
	eina-dock.h \
	dock.c

plugin_DATA = dock.ini

EXTRA_DIST = dock.ini
EXTRA_DIST += dock.ini
