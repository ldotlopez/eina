include $(top_srcdir)/Makefile.am.common

plugindir = $(pkglibdir)/playlist
plugin_LTLIBRARIES = libplaylist.la
libplaylist_la_CFLAGS  = @EINA_CFLAGS@
libplaylist_la_LDFLAGS = @EINA_LIBS@
libplaylist_la_SOURCES = \
	eina-playlist-ui.h \
	eina-playlist-marshallers.c \
	eina-playlist-marshallers.h \
	eina-playlist.c \
	eina-playlist.h \
	playlist.c

plugin_DATA = playlist.ini

glib_marshallers_list = eina-playlist-marshallers.list
glib_marshallers_prefix = eina_playlist_marshall
include $(top_srcdir)/Makefile.am.marshallers

eina_ui2h_ui = eina-playlist.ui
eina_ui2h_name = __ui_xml
include $(top_srcdir)/Makefile.am.ui2h

