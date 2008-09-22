BUILT_SOURCES = player.gtkui playlist.gtkui
CLEANFILES = $(BUILT_SOURCES)

player.gtkui: player.glade
	$(GTK_BUILDER_CONVERT) $< $@

playlist.gtkui: playlist.glade
	$(GTK_BUILDER_CONVERT) $< $@

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	player.glade \
	playlist.glade

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	for ui in `find . -iname '*.gtkui'`; do \
		$(INSTALL_DATA) $$ui $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	done

uninstall-local:	
	for ui in `find . -iname '*.gtkui'`; do \
		rm -- $(DESTDIR)$(datadir)/$(PACKAGE)/ui/$$ui; \
	done

