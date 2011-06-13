include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/preferences
plugin_LTLIBRARIES = libpreferences.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/player
include_HEADERS = \
	eina-preferences-dialog.h \
	eina-preferences-tab.h

libpreferences_la_CFLAGS  = @EINA_CFLAGS@
libpreferences_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libpreferences_la_SOURCES = \
	$(include_HEADERS)        \
	eina-preferences-dialog.c \
	eina-preferences-tab.c    \
	eina-preferences-plugin.h \
	eina-preferences-plugin.c

plugin_DATA = preferences.plugin
EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
