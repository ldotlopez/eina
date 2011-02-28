include $(top_srcdir)/build/Makefile.am.common

noinst_LTLIBRARIES = libeinaext.la

libeinaext_la_LDFLAGS = @EINA_LIBS@ -L$(top_builddir)/gel/.libs -L$(top_builddir)/lomo/.libs -lgel-ui-@GEL_API_VERSION@ -llomo-@LOMO_API_VERSION@
libeinaext_la_CFLAGS = @EINA_CFLAGS@
includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/ext
include_HEADERS = \
	curl-engine.h              \
	eina-application.h         \
	eina-file-chooser-dialog.h \
	eina-file-utils.h          \
	eina-fs.h                  \
	eina-stock.h               \
	eina-window.h

libeinaext_la_SOURCES = \
	$(include_HEADERS)         \
	eina-application.c         \
	curl-engine.c              \
	eina-file-chooser-dialog.c \
	eina-file-utils.c          \
	eina-fs.c                  \
	eina-stock.c               \
	eina-window.c

DISTCHECK_CONFIGURE_FLAGS = --enable-introspection

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_builddir)/gel
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_builddir)/gel

if HAVE_INTROSPECTION
introspection_sources = $(libeinaext_la_SOURCES)

Eina-0.10.gir: libeinaext.la
Eina_0_10_gir_INCLUDES = Gtk-3.0 Gel-@GEL_API_VERSION@
Eina_0_10_gir_CFLAGS = -I$(top_srcdir)
Eina_0_10_gir_LDFLAGS = -L$(top_builddir)/gel -lgel-@GEL_API_VERSION@ -lgel-ui-@GEL_API_VERSION@
Eina_0_10_gir_LIBS = libeinaext.la
Eina_0_10_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += Eina-0.10.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

-include $(top_srcdir)/git.mk
