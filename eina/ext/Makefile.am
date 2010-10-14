include $(top_srcdir)/build/Makefile.am.common

noinst_LTLIBRARIES = libeinaext.la

libeinaext_la_LDFLAGS = @EINA_LIBS@
libeinaext_la_CFLAGS = @EINA_CFLAGS@
includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/ext

include_HEADERS = \
	curl-engine.h \
	eina-conf.h \
	eina-cover.h \
	eina-cover-image.h \
	eina-file-chooser-dialog.h \
	eina-file-utils.h \
	eina-plugin-dialog.h \
	eina-plugin-properties.h \
	eina-preferences-dialog.h \
	eina-seek.h \
	eina-stock.h \
	eina-volume.h \
	eina-window.h

libeinaext_la_SOURCES = \
	curl-engine.c \
	eina-conf-file.c \
	eina-cover.c \
	eina-cover-image.c \
	eina-file-chooser-dialog.c \
	eina-file-utils.c \
	eina-plugin-dialog.c \
	eina-plugin-properties.c \
	eina-preferences-dialog.c \
	eina-preferences-dialog-marshallers.c \
	eina-preferences-dialog-marshallers.h \
	eina-seek.c \
	eina-stock.c \
	eina-volume.c \
	eina-window.c

#if USE_CLUTTER_GTK
#libeinaext_la_SOURCES += eina-cover-clutter.c
#include_HEADERS += eina-cover-clutter.h
#endif

EXTRA_DIST += \
	eina-plugin-dialog.ui \
	eina-plugin-properties.ui \
	eina-preferences-dialog-marshallers.list

BUILT_SOURCES += \
	eina-cover-image-mask.h \
	eina-preferences-dialog-marshallers.c \
	eina-preferences-dialog-marshallers.h \
	eina-plugin-dialog-ui.h \
	eina-plugin-properties-ui.h

glib_marshallers_list = eina-preferences-dialog-marshallers.list
glib_marshallers_prefix = eina_preferences_dialog_marshal
include $(top_srcdir)/build/Makefile.am.marshallers

eina-plugin-dialog-ui.h: eina-plugin-dialog.ui
	$(top_srcdir)/build/ui2h $< ui_xml > $@
 
eina-plugin-properties-ui.h: eina-plugin-properties.ui
	$(top_srcdir)/build/ui2h $< ui_xml > $@

eina-cover-image-mask.h: $(top_srcdir)/pixmaps/cover-mask.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_mask $< > $@

