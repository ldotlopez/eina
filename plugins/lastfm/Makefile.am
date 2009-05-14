plugindir = $(libdir)/$(PACKAGE)/lastfm

plugin_LTLIBRARIES = liblastfm.la

liblastfm_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
#if USE_LASTFM_WEBVIEW
#liblastfm_la_LDFLAGS += @WEBKITGTK_LIBS@
#endif

liblastfm_la_SOURCES = lastfm.c lastfm.h submit.c submit.h artwork.c artwork.h 
#if USE_LASTFM_WEBVIEW
#liblastfm_la_SOURCES += webview.c webview.h
#endif

AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)
#if USE_LASTFM_WEBVIEW
#AM_CFLAGS += -DHAVE_WEBKIT=1 @WEBKITGTK_CFLAGS@
#else
#AM_CFLAGS += -DHAVE_WEBKIT=0
#endif


lastfm.ui: lastfm.glade
	$(GTK_BUILDER_CONVERT) $< $@

BUILT_SOURCES = lastfm.ui
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	lastfm.png \
	lastfm.glade

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png' -o -iname '*.ui'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png' -o -iname '*.ui'` ; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

