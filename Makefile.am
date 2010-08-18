SUBDIRS = \
	build   \
	gel     \
	lomo    \
	eina    \
	plugins \
	pixmaps \
	data    \
	docs    \
	icons   \
	po

EXTRA_DIST = \
	run-uninstalled.sh \
	intltool-extract.in intltool-merge.in intltool-update.in \
	AUTHORS BUGS COPYING INSTALL NEWS README

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-instrospection --enable-gtk-doc

#
# Introspection support
#
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources = \
	lomo/lomo-stream.c \
	lomo/lomo-stream.h \
	lomo/lomo-metadata-parser.c \
	lomo/lomo-metadata-parser.h

Lomo-1.0.gir: lomo/liblomo-1.0.la
Lomo_1_0_gir_INCLUDES = Gst-0.10 GLib-2.0 GstBase-0.10
Lomo_1_0_gir_LIBS  = lomo/liblomo-1.0.la
Lomo_1_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += Lomo-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(gir_DATA) $(typelib_DATA)
endif # End introspection

