EXTRA_DIST = \
	eina.desktop.in 

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/applications
	$(INSTALL_DATA) eina.desktop $(DESTDIR)$(datadir)/applications/eina.desktop

uninstall-local:
	rm -- $(DESTDIR)$(datadir)/applications/eina.desktop

