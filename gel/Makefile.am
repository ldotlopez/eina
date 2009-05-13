AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGEL_COMPILATION \
	@PACKAGE_CFLAGS@

#	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
#	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
#	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
#

# Version X.Y must be specified
lib_LTLIBRARIES = libgel-1.0.la
libgel_1_0_la_LDFLAGS = -export-dynamic -version-info 0:90:0 -export-dynamic 
libgel_1_0_la_LIBADD  = @PACKAGE_LIBS@

NOINST_H_FILES = \
	gel-app-marshal.h

INST_H_FILES = \
	gel.h                   \
	gel-misc.h              \
	gel-string.h            \
	gel-str-parser.h        \
	gel-plugin.h            \
	gel-app.h               \
	gel-io-misc.h           \
	gel-io-op-result.h      \
	gel-io-ops.h            \
	gel-io-recurse-tree.h   \
	gel-io.h                \
	gel-ui.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/gel
header_DATA = $(INST_H_FILES)

libgel_1_0_la_SOURCES = \
	gel-misc.c              \
	gel-string.c            \
	gel-str-parser.c        \
	gel-app-marshal.c       \
	gel-plugin.c            \
	gel-app.c               \
	gel-io-misc.c           \
	gel-io-op-result.c      \
	gel-io-ops.c            \
	gel-io-recurse-tree.c   \
	gel-ui.c                \
	$(NOINST_H_FILES)       \
	$(INST_H_FILES)

BUILT_SOURCES = \
	gel-app-marshal.h \
	gel-app-marshal.c

CLEANFILES = $(BUILT_SOURCES)

gel-app-marshal.h: gel-app-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_app_marshal --header > $@

gel-app-marshal.c: gel-app-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_app_marshal --header --body > $@

EXTRA_DIST = gel-app-marshal.list

