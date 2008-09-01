ghub-marshal.h: ghub-marshal.list
	glib-genmarshal $< --prefix=ghub_marshal --header > $@

ghub-marshal.c: ghub-marshal.list
	glib-genmarshal $< --prefix=ghub_marshal --header --body > $@

BUILT_SOURCES = ghub-marshal.c ghub-marshal.h

EXTRA_DIST = \
    ghub-marshal.list

INCLUDES = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/libutil \
    -DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
    -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    @PACKAGE_CFLAGS@

noinst_LTLIBRARIES = libghub.la

libghub_la_SOURCES =         \
	ghub.c ghub.h ghub-marshal.c 

libghub_la_LDFLAGS = -export-dynamic

