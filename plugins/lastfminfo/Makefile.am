plugindir = $(libdir)/$(PACKAGE)/lastfminfo

plugin_LTLIBRARIES = liblastfminfo.la

liblastfminfo_la_LDFLAGS = @PACKAGE_LIBS@ @WEBKITGTK_LIBS@ -module -avoid-version
liblastfminfo_la_SOURCES = lastfm-info.c

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)
AM_CFLAGS = @PACKAGE_CFLAGS@ @WEBKITGTK_CFLAGS@

EXTRA_DIST = lastfm-icon.png

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png'`; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

