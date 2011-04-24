include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = liblomo-1.1.la
liblomo_1_1_la_LDFLAGS = -export-dynamic -version-info 1:1:1  -export-dynamic @GST_LIBS@
liblomo_1_1_la_CFLAGS = -DLOMO_COMPILATION @GST_CFLAGS@ @DEBUG_CFLAGS@

includedir = $(prefix)/include/lomo-1.1/lomo
include_HEADERS = \
	lomo.h                  \
	lomo-player.h           \
	lomo-metadata-parser.h  \
	lomo-stream.h           \
	lomo-util.h

lomodir = $(libdir)
liblomo_1_1_la_SOURCES = \
	$(include_HEADERS)     \
	lomo-stats.h           \
	lomo-playlist.h        \
	lomo-marshallers.c     \
	lomo-marshallers.h     \
	lomo-metadata-parser.c \
	lomo-player.c          \
	lomo-stats.c           \
	lomo-playlist.c        \
	lomo-stream.c          \
	lomo-util.c

glib_marshallers_list = lomo-marshallers.list
glib_marshallers_prefix = lomo_marshal
include $(top_srcdir)/build/Makefile.am.marshallers

-include $(top_srcdir)/git.mk
