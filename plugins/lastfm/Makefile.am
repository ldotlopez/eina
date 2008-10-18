plugindir = $(libdir)/$(PACKAGE)/lastfm

plugin_LTLIBRARIES = liblastfm.la
liblastfm_la_LDFLAGS = @PACKAGE_LIBS@ @LIBNOTIFY_LIBS@ -module -avoid-version
liblastfm_la_SOURCES = lastfm.c

AM_CFLAGS = @PACKAGE_CFLAGS@ @LIBNOTIFY_CFLAGS@
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)

lastfm.ui: lastfm.glade
	$(GTK_BUILDER_CONVERT) $< $@

BUILT_SOURCES = lastfm.ui
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	lastfm.glade

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png' lastfm.ui`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png'` lastfm.ui ; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

