include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/player
plugin_LTLIBRARIES = libplayer.la
libplayer_la_CFLAGS  = @EINA_CFLAGS@
libplayer_la_LDFLAGS = @EINA_LIBS@
libplayer_la_SOURCES = \
	eina-cover.c \
	eina-cover.h \
	eina-cover-image.c \
	eina-cover-image.h \
	eina-seek.c \
	eina-seek.h \
	eina-player-ui.h \
	eina-player-marshallers.h \
	eina-player-marshallers.c \
	eina-player.c \
	eina-player.h \
	eina-player-ui.h \
	player.c

plugin_DATA = player.ini

eina-cover-default-cover.h: $(top_srcdir)/pixmaps/cover-default.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_default_png $<  > $@

eina-cover-loading-cover.h: $(top_srcdir)/pixmaps/cover-loading.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_loading_png $<  > $@

eina-cover-image-mask.h: $(top_srcdir)/pixmaps/cover-mask.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_mask_png $<  > $@

png_h_files = \
	eina-cover-image-mask.h \
	eina-cover-default-cover.h \
	eina-cover-loading-cover.h
CLEANFILES += $(png_h_files)
DISTCLEANFILES += $(png_h_files)
BUILT_SOURCES += $(png_h_files)
EXTRA_DIST += $(png_h_files)
MAINTAINERCLEANFILES += $(png_h_files)
libplayer_la_SOURCES += $(png_h_files)

eina_ui2h_ui = eina-player.ui
eina_ui2h_name = __eina_player_ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

glib_marshallers_list = eina-player-marshallers.list
glib_marshallers_prefix = eina_player_marshall
include $(top_srcdir)/build/Makefile.am.marshallers

