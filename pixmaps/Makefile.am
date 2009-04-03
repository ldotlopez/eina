# :r ! /bin/ls -1 | grep -v Makefile | sed -e 's/^\(.*\)$/\t\1 \\/'
EXTRA_DIST = \
	audio-volume-high.png \
	audio-volume-low.png \
	audio-volume-medium.png \
	audio-volume-mute.png \
	cover-default.png \
	cover-loading.png \
	cover-mask.png \
	eina-queue.png \
	icon.png \
	icon-512.png \
	logo.png \
	random.png \
	repeat.png \
	vogon-icon.png

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/pixmaps; \
	for p in `find . -iname '*.png'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)$(datadir)/$(PACKAGE)/pixmaps; \
	done

uninstall-local:	
	for p in `find . -iname '*.png'`; do \
		rm -- $(DESTDIR)$(datadir)/$(PACKAGE)/pixmaps/$$p; \
	done

