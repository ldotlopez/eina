include $(top_srcdir)/build/Makefile.am.common

VALAFLAGS=--pkg gtk+-3.0
vala_files=eina-dock-tab.vala
include $(top_srcdir)/build/Makefile.am.vala

plugindir = $(pkglibdir)/dock
plugin_LTLIBRARIES = libdock.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/dock
include_HEADERS = \
	eina-dock.h     \
	eina-dock-tab.h \
	dock.h

libdock_la_CFLAGS  = @EINA_CFLAGS@
libdock_la_LDFLAGS = @EINA_LIBS@
libdock_la_SOURCES =   \
	$(include_HEADERS) \
	eina-dock.c        \
	eina-dock-tab.c    \
	dock.c

plugin_DATA = dock.ini

EXTRA_DIST += dock.ini
