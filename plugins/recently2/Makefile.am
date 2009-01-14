plugindir = $(libdir)/$(PACKAGE)/recently2

plugin_LTLIBRARIES = librecently2.la
librecently2_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
librecently2_la_SOURCES = iface.c

AM_CFLAGS = @PACKAGE_CFLAGS@
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)

#recently.ui: recently.glade
#	$(GTK_BUILDER_CONVERT) $< $@
#
#BUILT_SOURCES = recently.ui
#CLEANFILES = $(BUILT_SOURCES)
#
#EXTRA_DIST = \
#	$(BUILT_SOURCES) \
#	recently.png \
#	recently.glade

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png' -o -iname '*.ui'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png' -o -iname '*.ui'` ; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

