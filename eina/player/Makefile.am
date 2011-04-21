include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/player
plugin_LTLIBRARIES = libplayer.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/player
include_HEADERS = \
	eina-cover.h       \
	eina-cover-image.h \
	eina-seek.h        \
	eina-player.h      \
	player.h

libplayer_la_CFLAGS  = @EINA_CFLAGS@
libplayer_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libplayer_la_SOURCES = \
	$(include_HEADERS) \
	eina-cover.c       \
	eina-cover-image.c \
	eina-seek.c        \
	eina-player.c      \
	player.c

plugin_DATA = preferences.ui player.ini

eina-cover-image-mask.h: $(top_srcdir)/pixmaps/cover-mask.png
	$(AM_V_GEN)$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_mask_png $<  > $@

png_h_files = \
	eina-cover-image-mask.h

CLEANFILES += $(png_h_files)
DISTCLEANFILES += $(png_h_files)
BUILT_SOURCES += $(png_h_files)
EXTRA_DIST += $(png_h_files)
MAINTAINERCLEANFILES += $(png_h_files)
libplayer_la_SOURCES += $(png_h_files)

eina_ui2h_ui = eina-player.ui
eina_ui2h_name = __eina_player_ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

EXTRA_DIST += $(plugin_DATA) player.ini

-include $(top_srcdir)/git.mk
