include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = libgel-2.0.la libgel-ui-2.0.la

libgel_2_0_la_CFLAGS  = -DGEL_COMPILATION @GLIB_CFLAGS@ @GTK_CFLAGS@ @DEBUG_CFLAGS@ 
libgel_2_0_la_LDFLAGS = -export-dynamic -version-info 2:0:0 -export-dynamic @GLIB_LIBS@

libgel_ui_2_0_la_CFLAGS  = -DGEL_UI_COMPILATION @GLIB_CFLAGS@ @GTK_CFLAGS@ @DEBUG_CFLAGS@
libgel_ui_2_0_la_LDFLAGS = $(libgel_2_0_la_LDFLAGS) @GTK_LIBS@

common_includedir = $(prefix)/include/gel-@GEL_API_VERSION@/gel

libgel_includedir = $(common_includedir)
libgel_include_HEADERS = \
	gel.h                   \
	gel-misc.h              \
	gel-str-parser.h        \
	gel-io.h                \
	gel-fs-scanner.h        \
	gel-io-scanner.h

libgel_ui_includedir = $(common_includedir)
libgel_ui_include_HEADERS = \
	gel-ui.h                \
	gel-ui-dialogs.h        \
	gel-ui-generic.h        \
	gel-ui-utils.h          \
	gel-ui-scale.h

common_geldir = $(libdir)/gel-@GEL_API_VERSION@

libgeldir = $(common_geldir)
libgel_2_0_la_SOURCES = \
	$(libgel_include_HEADERS) \
	gel-misc.c                \
	gel-str-parser.c          \
	gel-marshallers.c         \
	gel-fs-scanner.c          \
	gel-io-scanner.c

libgel_uidir = $(common_geldir)
libgel_ui_2_0_la_SOURCES = \
	$(libgel_ui_include_HEADERS) \
	gel-ui-dialogs.c             \
	gel-ui-generic.c             \
	gel-ui-utils.c               \
	gel-ui-scale.c

glib_marshallers_list = gel-marshallers.list
glib_marshallers_prefix = gel_marshal
include $(top_srcdir)/build/Makefile.am.marshallers

#
# Introspection stuff
#
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =

INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_builddir)/gel --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_builddir)/gel

Gel-2.0.gir: libgel-2.0.la
Gel_2_0_gir_INCLUDES = GObject-2.0 Gio-2.0
Gel_2_0_gir_CFLAGS = -I$(top_srcdir)
Gel_2_0_gir_LIBS = libgel-2.0.la
Gel_2_0_gir_FILES = $(libgel_2_0_la_SOURCES)
INTROSPECTION_GIRS += Gel-2.0.gir

GelUI-2.0.gir: libgel-ui-2.0.la
GelUI_2_0_gir_INCLUDES = GObject-2.0 Gio-2.0 Gtk-3.0 Gel-2.0
GelUI_2_0_gir_CFLAGS = -I$(top_srcdir)
GelUI_2_0_gir_LDFLAGS = -L$(top_builddir)/gel/.libs -lgel-2.0
GelUI_2_0_gir_LIBS = libgel-ui-2.0.la
GelUI_2_0_gir_FILES = $(libgel_ui_2_0_la_SOURCES)
INTROSPECTION_GIRS += GelUI-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

#
# Misc
#
CLEANFILES += $(gir_DATA) $(typelib_DATA)
-include $(top_srcdir)/git.mk

