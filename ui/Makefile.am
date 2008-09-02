# :r ! /bin/ls -1 | sed -e 's/^\(.*\)$/\t\1 \\/' | grep -v Makefile
EXTRA_DIST = \
	player.glade \
	player.gladep \
	player.gtkui \
	playlist.glade \
	playlist.gladep \
	playlist.gtkui

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	for ui in `find . -iname '*.gtkui'`; do \
		$(INSTALL_DATA) $$ui $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	done

uninstall-local:	
	for ui in `find . -iname '*.gtkui'`; do \
		rm -- $(DESTDIR)$(datadir)/$(PACKAGE)/ui/$$ui; \
	done

BUILT_SOURCES = player.gtkui playlist.gtkui

player.gtkui: player.glade
	gtk-builder-convert $< $@

playlist.gtkui: playlist.glade
	gtk-builder-convert $< $@

