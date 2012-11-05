include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/player
plugin_LTLIBRARIES = libplayer.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/player
include_HEADERS = \
	eina-cover.h       \
	eina-cover-image.h \
	eina-seek.h        \
	eina-player.h      \
	eina-player-plugin.h

libplayer_la_CFLAGS  = @EINA_CFLAGS@
libplayer_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libplayer_la_SOURCES = \
	$(include_HEADERS) \
	eina-cover.c       \
	eina-cover-image.c \
	eina-seek.c        \
	eina-player.c      \
	eina-player-plugin.c \
	resources.c

plugin_DATA = player.plugin

eina_gresources_xml = resources.xml
eina_gresources_name = eina_player
include $(top_srcdir)/build/Makefile.am.gresources

EXTRA_DIST += \
	$(plugin_DATA)   \
	resources.xml.in \
	main.ui preferences.ui \
	cover-mask.png

-include $(top_srcdir)/git.mk
