# Build / clean / extra
BUILT_SOURCES = lomo-player-glue.h
CLEAN_FILES = lomo-player-glue.h
EXTRA_DIST = lomo-player-glue.h

lomo-player-glue.h: lomo-player.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=lomo_player $< > $@

# pkgconfig files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = eina-@EINA_API_VERSION@.pc

# desktop files
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/applications
	$(INSTALL_DATA) eina.desktop $(DESTDIR)$(datadir)/applications/eina.desktop

uninstall-local:
	rm -- $(DESTDIR)$(datadir)/applications/eina.desktop

