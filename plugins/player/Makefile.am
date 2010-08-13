include $(top_srcdir)/Makefile.am.common

plugindir = $(pkglibdir)/player
plugin_LTLIBRARIES = libplayer.la
libplayer_la_CFLAGS  = @EINA_CFLAGS@
libplayer_la_LDFLAGS = @EINA_LIBS@
libplayer_la_SOURCES = \
	eina-seek.c \
	eina-seek.h \
	eina-player-ui.h \
	eina-player-marshallers.h \
	eina-player-marshallers.c \
	eina-player.c \
	eina-player.h \
	player.c

plugin_DATA = player.ini

eina_ui2h_ui = eina-player.ui
eina_ui2h_name = __eina_player_ui_xml
include $(top_srcdir)/Makefile.am.ui2h

glib_marshallers_list = eina-player-marshallers.list
glib_marshallers_prefix = eina_player_marshall
include $(top_srcdir)/Makefile.am.marshallers

