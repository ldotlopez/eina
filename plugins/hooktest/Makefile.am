plugindir = $(libdir)/$(PACKAGE)/hooktest

plugin_LTLIBRARIES = libhooktest.la
libhooktest_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
libhooktest_la_SOURCES = hook-test.c

AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)/$(plugindir); \
	for p in `find . -iname '*.png'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)/$(plugindir); \
	done

uninstall-local:    
	for p in `find . -iname '*.png'`; do \
		rm -- $(DESTDIR)/$(plugindir)/$$p; \
	done

