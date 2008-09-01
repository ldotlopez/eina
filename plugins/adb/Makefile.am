plugindir = $(libdir)/$(PACKAGE)/adb

plugin_LTLIBRARIES = libadb.la
libadb_la_LDFLAGS = @PACKAGE_LIBS@ @GMODULE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
libadb_la_SOURCES = adb.c adb.h

AM_CFLAGS = @PACKAGE_CFLAGS@ @GMODULE_CFLAGS@ @SQLITE3_CFLAGS@
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

