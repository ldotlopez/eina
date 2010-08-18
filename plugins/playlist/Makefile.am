include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/playlist
plugin_LTLIBRARIES = libplaylist.la
libplaylist_la_CFLAGS  = @EINA_CFLAGS@
libplaylist_la_LDFLAGS = @EINA_LIBS@
libplaylist_la_SOURCES = \
	eina-playlist-ui.h \
	eina-playlist.c \
	eina-playlist.h \
	playlist.c

plugin_DATA = playlist.ini

eina_ui2h_ui = eina-playlist.ui
eina_ui2h_name = __ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

EXTRA_DIST = playlist.ini
