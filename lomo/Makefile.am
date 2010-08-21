include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = liblomo-1.0.la
liblomo_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0 -export-dynamic @GSTREAMER_LIBS@
liblomo_1_0_la_CFLAGS = -DLOMO_COMPILATION @GSTREAMER_CFLAGS@ @DEBUG_CFLAGS@
lomodir = $(prefix)/lib

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/lomo
include_HEADERS = \
	lomo-player.h           \
	lomo-metadata-parser.h  \
	lomo-stream.h           \
	lomo-util.h

liblomo_1_0_la_SOURCES = \
	lomo-marshallers.c     \
	lomo-marshallers.h     \
	lomo-metadata-parser.c \
	lomo-metadata-parser.h \
	lomo-player.c          \
	lomo-playlist.c        \
	lomo-playlist.h        \
	lomo-stream.c          \
	lomo-util.c

glib_marshallers_list = lomo-marshallers.list
glib_marshallers_prefix = lomo_marshal
include $(top_srcdir)/build/Makefile.am.marshallers

