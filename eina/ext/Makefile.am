include $(top_srcdir)/build/Makefile.am.common

noinst_LTLIBRARIES = libeinaext.la

libeinaext_la_LDFLAGS = @EINA_LIBS@ -L$(top_builddir)/gel/.libs -L$(top_builddir)/lomo/.libs -lgel-ui-@GEL_API_VERSION@ -llomo-@LOMO_API_VERSION@
libeinaext_la_CFLAGS = @EINA_CFLAGS@
includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/ext
include_HEADERS = \
	curl-engine.h              \
	eina-application.h         \
	eina-activatable.h         \
	eina-file-chooser-dialog.h \
	eina-file-utils.h          \
	eina-fs.h                  \
	eina-stock.h               \
	eina-window.h

libeinaext_la_SOURCES = \
	$(include_HEADERS)         \
	eina-application.c         \
	eina-activatable.c         \
	curl-engine.c              \
	eina-file-chooser-dialog.c \
	eina-file-utils.c          \
	eina-fs.c                  \
	eina-stock.c               \
	eina-window.c

#DISTCHECK_CONFIGURE_FLAGS = --enable-introspection

#-include $(INTROSPECTION_MAKEFILE)
#INTROSPECTION_GIRS =
#INTROSPECTION_SCANNER_ARGS = \
#	--warn-all \
#	--add-include-path=$(top_builddir)/lomo \
#	--add-include-path=$(top_builddir)/gel
#INTROSPECTION_COMPILER_ARGS = \
#	--includedir=$(top_builddir)/lomo \
#	--includedir=$(top_builddir)/gel
#
#if HAVE_INTROSPECTION
#Eina-0.12.gir: libeinaext.la
#Eina_0_12_gir_INCLUDES = Lomo-2.0 GelUI-2.0
#Eina_0_12_gir_CFLAGS   = $(INCLUDES) -I$(top_srcdir)
#Eina_0_12_gir_LDFLAGS  = -llomo-2.0 -lgel-2.0 -lgel-ui-2.0
#Eina_0_12_gir_LIBS     = libeinaext.la
##Eina_0_12_gir_FILES = $(eina_SOURCES) $(include_HEADERS) 
#Eina_0_12_gir_FILES = \
#	$(top_srcdir)/eina/ext/eina-window.h      \
#	$(top_srcdir)/eina/ext/eina-window.c      \
#	$(top_srcdir)/eina/ext/eina-application.h \
#	$(top_srcdir)/eina/ext/eina-application.c \
#	$(top_srcdir)/eina/ext/eina-activatable.h \
#	$(top_srcdir)/eina/ext/eina-activatable.c
#INTROSPECTION_GIRS += Eina-0.12.gir
#
#girdir = $(datadir)/@PACKAGE@/gir-1.0
#gir_DATA = $(INTROSPECTION_GIRS)
#
#typelibdir = $(libdir)/@PACKAGE@/girepository-1.0
#typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)
#
#CLEANFILES += $(gir_DATA) $(typelib_DATA)
#endif


-include $(top_srcdir)/git.mk
