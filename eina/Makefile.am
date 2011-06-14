include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = ext   \
	art         \
	dbus        \
	dock        \
	fieshta     \
	lastfm      \
	lomo        \
	mpris       \
	player      \
	playlist    \
	preferences \
	pythonhello \
	status-icon

#if BUILD_ADB
#SUBDIRS +=   \
#	adb      
#	muine
#	recently
#endif

#if BUILD_CLUTTY
#SUBDIRS += clutty
#endif

#if BUILD_NTFY
#SUBDIRS += ntfy
#endif

bin_PROGRAMS = eina
eina_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
eina_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
eina_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-@GEL_API_VERSION@ -lgel-ui-@GEL_API_VERSION@ -llomo-@LOMO_API_VERSION@

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
include_HEADERS = 


eina_SOURCES = \
	eina.c

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = \
	--warn-all \
	--add-include-path=$(top_builddir)/lomo \
	--add-include-path=$(top_builddir)/gel
INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(top_builddir)/lomo \
	--includedir=$(top_builddir)/gel

if HAVE_INTROSPECTION
Eina-0.12.gir: eina
Eina_0_12_gir_INCLUDES = Lomo-2.0 GelUI-2.0 Peas-1.0
Eina_0_12_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Eina_0_12_gir_PROGRAM = $(top_builddir)/eina/eina
Eina_0_12_gir_LDADD = $(top_builddir)/lomo/liblomo.la
Eina_0_12_gir_FILES = $(eina_SOURCES) $(include_HEADERS) \
	$(top_srcdir)/eina/ext/eina-application.h \
	$(top_srcdir)/eina/ext/eina-application.c \
	$(top_srcdir)/eina/ext/eina-activatable.h \
	$(top_srcdir)/eina/ext/eina-activatable.c \
	$(top_srcdir)/eina/ext/eina-window.h \
	$(top_srcdir)/eina/ext/eina-window.c \
	$(top_srcdir)/eina/lomo/eina-lomo-plugin.h \
	$(top_srcdir)/eina/lomo/eina-lomo-plugin.c 
INTROSPECTION_GIRS += Eina-0.12.gir

girdir = $(datadir)/@PACKAGE@/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/@PACKAGE@/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

-include $(top_srcdir)/git.mk
