plugindir = $(libdir)/$(PACKAGE)/recently

plugin_LTLIBRARIES = librecently.la
librecently_la_LDFLAGS = @PACKAGE_LIBS@ @GMODULE_LIBS@ -module -avoid-version
librecently_la_SOURCES = \
	scores.c   scores.h \
	recently.c recently.h

AM_CFLAGS = @PACKAGE_CFLAGS@ @GMODULE_CFLAGS@
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

