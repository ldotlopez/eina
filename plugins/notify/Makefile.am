plugindir = $(libdir)/$(PACKAGE)/notify

plugin_LTLIBRARIES = libnotify.la
libnotify_la_LDFLAGS = @PACKAGE_LIBS@ @LIBNOTIFY_LIBS@ -module -avoid-version
libnotify_la_SOURCES = notify.c notify.h

AM_CFLAGS = @PACKAGE_CFLAGS@ @LIBNOTIFY_CFLAGS@
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png'`; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

