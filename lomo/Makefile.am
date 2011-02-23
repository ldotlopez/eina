include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = liblomo-1.1.la
liblomo_1_1_la_LDFLAGS = -export-dynamic -version-info 1:1:0  -export-dynamic @GST_LIBS@
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

## Vala bindings
## /opt/gtk/bin/vala-gen-introspect  lomo-1.1 .
## /opt/gtk/bin/vapigen-0.12 --pkg Gst-0.10 --pkg GLib-2.0 --library lomo-1.1  lomo-1.1.gi
# $(VALA_GEN_INTROSPECT) lomo-1.1 $(top_builddir)/lomo
# $(VAPIGEN) --pkg GLib-@GLIB_API_VERSION@ --pkg Gst-@GST_API_VERSION@ --library Lomo-1.1 lomo-1.1.gi

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS = 

if HAVE_INTROSPECTION
Lomo-1.1.gir: liblomo-1.1.la
Lomo_1_1_gir_INCLUDES = Gst-@GST_API_VERSION@
Lomo_1_1_gir_PACKAGES = gstreamer-@GST_API_VERSION@
Lomo_1_1_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Lomo_1_1_gir_LIBS = liblomo-1.1.la
Lomo_1_1_gir_FILES = \
	lomo-player.h lomo-stream.h lomo-util.h \
	lomo-player.c lomo-stream.c lomo-util.c
INTROSPECTION_GIRS += Lomo-1.1.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

-include $(top_srcdir)/git.mk
