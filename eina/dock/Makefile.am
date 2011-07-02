include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/dock
plugin_LTLIBRARIES = libdock.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/dock
include_HEADERS = \
	eina-dock.h     \
	eina-dock-tab.h \
	eina-dock-plugin.h

libdock_la_CFLAGS  = @EINA_CFLAGS@
libdock_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libdock_la_SOURCES =   \
	$(include_HEADERS) \
	eina-dock.c        \
	eina-dock-tab.c    \
	eina-dock-plugin.c

plugin_DATA = dock.plugin

EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
