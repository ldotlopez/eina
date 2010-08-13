include $(top_srcdir)/Makefile.am.common

noinst_LTLIBRARIES = libeinaext.la

libeinaext_la_LDFLAGS = @EINA_LIBS@
libeinaext_la_CFLAGS = @EINA_CFLAGS@
includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/ext
include_HEADERS = \
	curl-engine.h              \
	eina-cover.h               \
	eina-cover-image.h         \
	eina-file-chooser-dialog.h \
	eina-file-utils.h          \
	eina-preferences-dialog.h  \
	eina-preferences-tab.h     \
	eina-seek.h                \
	eina-stock.h               \
	eina-window.h

libeinaext_la_SOURCES = \
	curl-engine.c              \
	eina-cover.c               \
	eina-cover-image.c         \
	eina-file-chooser-dialog.c \
	eina-file-utils.c          \
	eina-preferences-dialog.c  \
	eina-preferences-tab.c     \
	eina-seek.c                \
	eina-stock.c               \
	eina-window.c

BUILT_SOURCES += \
	eina-cover-image-mask.h

eina-cover-image-mask.h: $(top_srcdir)/pixmaps/cover-mask.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_mask $<  > $@

