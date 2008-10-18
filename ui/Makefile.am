BUILT_SOURCES = player.ui playlist.ui about.ui plugins.ui
CLEANFILES = $(BUILT_SOURCES)

player.ui: player.glade
	$(GTK_BUILDER_CONVERT) $< $@

playlist.ui: playlist.glade
	$(GTK_BUILDER_CONVERT) $< $@

about.ui: about.glade
	$(GTK_BUILDER_CONVERT) $< $@

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	about.glade      \
	player.glade     \
	player-menu.ui   \
	playlist.glade  \
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

