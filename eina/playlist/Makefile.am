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
	$(include_HEADERS)     \
	eina-playlist.c        \
	eina-playlist-plugin.c \
	resources.c

plugin_DATA = playlist.plugin

eina_gresources_xml = resources.xml
eina_gresources_name = eina_playlist
include $(top_srcdir)/build/Makefile.am.gresources

EXTRA_DIST += \
	$(plugin_DATA)   \
	resources.xml.in \
	main.ui

-include $(top_srcdir)/git.mk
