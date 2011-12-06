include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = liblomo-2.0.la
liblomo_2_0_la_LDFLAGS = -export-dynamic -version-info 2:0:0 -L$(top_builddir)/gel/.libs -lgel-2.0 @GST_LIBS@
liblomo_2_0_la_CFLAGS = -DLOMO_COMPILATION @GST_CFLAGS@ @DEBUG_CFLAGS@

includedir = $(prefix)/include/lomo-2.0/lomo
include_HEADERS = \
	lomo.h                  \
	lomo-em-art.h           \
	lomo-em-art-backend.h   \
	lomo-em-art-backends.h  \
	lomo-em-art-provider.h  \
	lomo-em-art-search.h    \
	lomo-player.h           \
	lomo-metadata-parser.h  \
	lomo-stream.h           \
	lomo-logger.h \
	lomo-util.h

lomodir = $(libdir)
liblomo_2_0_la_SOURCES = \
	$(include_HEADERS)     \
	lomo-em-art.c          \
	lomo-em-art-backend.c  \
	lomo-em-art-backends.c \
	lomo-em-art-provider.c \
	lomo-em-art-search.c   \
	lomo-logger.c          \
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

## Vala bindings
## /opt/gtk/bin/vala-gen-introspect  lomo-2.0 .
# /opt/gtk/bin/vapigen-0.12 --pkg Gst-0.10 --pkg GLib-2.0 --library lomo-2.0  lomo-2.0.gi
#lomo-2.0.gi:
#	$(VALA_GEN_INTROSPECT) lomo-2.0 $(top_builddir)/lomo
#	$(VAPIGEN) --pkg GLib-@GLIB_API_VERSION@ --pkg Gst-@GST_API_VERSION@ --library Lomo-2.0 lomo-2.0.gi
#
#BUILT_SOURCES += lomo-2.0.gi

include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =

Lomo-2.0.gir: liblomo-2.0.la
Lomo_2_0_gir_INCLUDES = Gst-@GST_API_VERSION@
Lomo_2_0_gir_PACKAGES = gstreamer-@GST_API_VERSION@
Lomo_2_0_gir_LDADD  = -L$(top_builddir)/gel/.libs -lgel-2.0
Lomo_2_0_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Lomo_2_0_gir_LIBS = liblomo-2.0.la
Lomo_2_0_gir_FILES = \
	lomo-player.h lomo-stream.h lomo-metadata-parser.h lomo-util.h \
	lomo-em-art.h lomo-em-art-backend.h lomo-em-art-backends.h lomo-em-art-provider.h lomo-em-art-search.h \
    \
	lomo-player.c lomo-stream.c lomo-metadata-parser.c lomo-util.c \
	lomo-em-art.c lomo-em-art-backend.c lomo-em-art-backends.c lomo-em-art-provider.c lomo-em-art-search.c

INTROSPECTION_GIRS += Lomo-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

-include $(top_srcdir)/git.mk
