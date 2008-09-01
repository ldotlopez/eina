# :r ! /bin/ls -1 | grep -v Makefile | sed -e 's/^\(.*\)$/\t\1 \\/'
EXTRA_DIST = \
	audio-volume-high.png \
	audio-volume-low.png \
	audio-volume-medium.png \
	audio-volume-mute.png \
	avatar-inet.png \
	avatar.png \
	avatar-zzz.png \
	cover-unknow.png \
	cover-unknow.xcf \
	h1.png \
	h2.png \
	icon.png \
	logo.png \
	logo.svg \
	logo-v1.xcf \
	queue.png \
	random.png \
	random-repeat.svg \
	repeat.png \
	vogon-icon.png \
	volume.xcf

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/pixmaps; \
	for p in `find . -iname '*.png'`; do \
		$(INSTALL_DATA) $$p $(DESTDIR)$(datadir)/$(PACKAGE)/pixmaps; \
	done

uninstall-local:	
	for p in `find . -iname '*.png'`; do \
		rm -- $(DESTDIR)$(datadir)/$(PACKAGE)/pixmaps/$$p; \
	done

