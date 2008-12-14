plugindir = $(libdir)/$(PACKAGE)/coverplus

plugin_LTLIBRARIES = libcoverplus.la
libcoverplus_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
libcoverplus_la_SOURCES = \
	iface.c \
	infolder-sync.c infolder.h \
	banshee.h banshee.c

AM_CFLAGS = @PACKAGE_CFLAGS@ 
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

