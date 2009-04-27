EXTRA_DIST = \
	about.ui              \
	dock.ui               \
	log.ui                \
	player-menu.ui        \
	player-preferences.ui \
	player.ui             \
	playlist.ui           \
	plugins.ui

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	for ui in `find . -iname '*.ui'`; do \
		$(INSTALL_DATA) $$ui $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	done

uninstall-local:	
	for ui in `find . -iname '*.ui'`; do \
		rm -- $(DESTDIR)$(datadir)/$(PACKAGE)/ui/$$ui; \
	done

