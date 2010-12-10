include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = liblomo-1.0.la
liblomo_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0 -export-dynamic @GSTREAMER_LIBS@
liblomo_1_0_la_CFLAGS = -DLOMO_COMPILATION @GSTREAMER_CFLAGS@ @DEBUG_CFLAGS@
lomodir = $(prefix)/lib

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/lomo
include_HEADERS = \
	lomo.h                  \
	lomo-player.h           \
	lomo-metadata-parser.h  \
	lomo-stream.h           \
	lomo-util.h
nodist_include_HEADERS = \
	lomo-stats.h          \
	lomo-playlist.h

liblomo_1_0_la_SOURCES = \
	$(include_HEADERS)     \
	$(nodist_include_HEADERS) \
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

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = 
INTROSPECTION_COMPILER_ARGS = 

if HAVE_INTROSPECTION
Lomo-1.0.gir: liblomo-1.0.la
Lomo_1_0_gir_INCLUDES = Gst-0.10
Lomo_1_0_gir_PACKAGES = gstreamer-0.10
Lomo_1_0_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Lomo_1_0_gir_LIBS = liblomo-1.0.la
Lomo_1_0_gir_FILES = $(liblomo_1_0_la_SOURCES)
INTROSPECTION_GIRS += Lomo-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
