include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/playlist
plugin_LTLIBRARIES = libplaylist.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/playlist
include_HEADERS = \
	eina-playlist.h \
	playlist.h

libplaylist_la_CFLAGS  = @EINA_CFLAGS@
libplaylist_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libplaylist_la_SOURCES = \
	$(include_HEADERS) \
	eina-playlist.c \
	playlist.c

plugin_DATA = playlist.ini

eina_ui2h_ui = eina-playlist.ui
eina_ui2h_name = __ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

EXTRA_DIST += playlist.ini

-include $(top_srcdir)/build/git.mk
