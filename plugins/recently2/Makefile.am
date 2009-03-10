plugindir = $(libdir)/$(PACKAGE)/recently2

plugin_LTLIBRARIES = librecently2.la
librecently2_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
librecently2_la_SOURCES = recently.c

AM_CFLAGS = @PACKAGE_CFLAGS@
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)

dock.ui: dock.glade
	$(GTK_BUILDER_CONVERT) $< $@

BUILT_SOURCES = dock.ui
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	dock.glade
#	recently.png 

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png' -o -iname '*.ui'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png' -o -iname '*.ui'` ; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

