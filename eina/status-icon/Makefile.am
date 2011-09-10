include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/status-icon
plugin_LTLIBRARIES = libstatus-icon.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/status-icon
include_HEADERS =

libstatus_icon_la_CFLAGS  = @EINA_CFLAGS@
libstatus_icon_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libstatus_icon_la_SOURCES = \
	eina-status-icon-plugin.h \
	eina-status-icon-plugin.c

plugin_DATA = status-icon.plugin

EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
