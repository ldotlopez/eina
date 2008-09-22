pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = eina-@EINA_API_VERSION@.pc

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/applications
	$(INSTALL_DATA) eina.desktop $(DESTDIR)$(datadir)/applications/eina.desktop

uninstall-local:
	rm -- $(DESTDIR)$(datadir)/applications/eina.desktop

