BUILT_SOURCES = about.ui dock.ui player.ui player-preferences.ui playlist.ui plugins.ui

CLEANFILES = $(BUILT_SOURCES)

about.ui: about.glade
	$(GTK_BUILDER_CONVERT) $< $@

dock.ui: dock.glade
	$(GTK_BUILDER_CONVERT) $< $@

player.ui: player.glade
	$(GTK_BUILDER_CONVERT) $< $@

player-preferences.ui: player-preferences.glade
	$(GTK_BUILDER_CONVERT) $< $@

playlist.ui: playlist.glade
	$(GTK_BUILDER_CONVERT) $< $@

plugins.ui: plugins.glade
	$(GTK_BUILDER_CONVERT) $< $@

EXTRA_DIST = \
	$(BUILT_SOURCES)         \
	about.glade              \
	dock.glade               \
	player-menu.ui           \
	player.glade             \
	player-preferences.glade \
	playlist.glade           \
	plugins.glade

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	for ui in `find . -iname '*.ui'`; do \
		$(INSTALL_DATA) $$ui $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	done

uninstall-local:	
	for ui in `find . -iname '*.ui'`; do \
		rm -- $(DESTDIR)$(datadir)/$(PACKAGE)/ui/$$ui; \
	done

