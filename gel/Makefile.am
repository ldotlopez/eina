## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

gel-hub-marshal.h: gel-hub-marshal.list
	glib-genmarshal $< --prefix=gel_hub_marshal --header > $@

gel-hub-marshal.c: gel-hub-marshal.list
	glib-genmarshal $< --prefix=gel_hub_marshal --header --body > $@

BUILT_SOURCES = gel-hub-marshal.c gel-hub-marshal.h

EXTRA_DIST = \
	gel-hub-marshal.list

noinst_LTLIBRARIES = libgel.la

libgel_la_SOURCES =         \
	gel.c \
	gel-hub.c gel.hub.h \
	gel-hub-marshal.c gel-hub-marshal.h \
	gel-ui.c \
	gel-io-async-read-op.c \
	gel.h \
	gel-io.h \
	gel-ui.h \
	gel-io-async-read-op.h

libgel_la_LDFLAGS = -export-dynamic

