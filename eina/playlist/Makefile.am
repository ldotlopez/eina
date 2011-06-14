include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/playlist
plugin_LTLIBRARIES = libplaylist.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/playlist
include_HEADERS = \
	eina-playlist.h \
	eina-playlist-plugin.h

libplaylist_la_CFLAGS  = @EINA_CFLAGS@
libplaylist_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libplaylist_la_SOURCES = \
	$(include_HEADERS) \
	eina-playlist.c \
	eina-playlist-plugin.c

plugin_DATA = playlist.plugin

eina_ui2h_ui = eina-playlist.ui
eina_ui2h_name = __ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
