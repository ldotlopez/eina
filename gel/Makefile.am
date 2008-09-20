INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

lib_LTLIBRARIES = libgel.la

libgel_la_LDFLAGS = -export-dynamic
libgel_la_LIBADD = @PACKAGE_LIBS@

NOINST_H_FILES = \
	gel-hub-marshal.h

INST_H_FILES = \
	gel.h                  \
	gel-str-parser.h       \
	gel-hub.h              \
	gel-io-async-read-op.h \
	gel-io-async-read-dir.h \
	gel-io.h               \
	gel-ui.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/gel
header_DATA = $(INST_H_FILES)

libgel_la_SOURCES = \
	gel.c                  \
	gel-str-parser.c       \
	gel-hub-marshal.c      \
	gel-hub.c              \
	gel-io-async-read-op.c \
	gel-io-async-read-dir.c \
	gel-ui.c               \
	$(NOINST_H_FILES)      \
	$(INST_H_FILES)

BUILT_SOURCES = \
	gel-hub-marshal.h \
	gel-hub-marshal.c

CLEANFILES = $(BUILT_SOURCES)

gel-hub-marshal.h: gel-hub-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_hub_marshal --header > $@

gel-hub-marshal.c: gel-hub-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_hub_marshal --header --body > $@

EXTRA_DIST = gel-hub-marshal.list

