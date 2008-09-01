## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = eina

# ether.{c,h} are template files to create GObjects
# template.{c,h} are template files to create Eina components
eina_SOURCES = \
	main.c                  \
	ether.c     ether.h     \
	template.c  template.h  \
    class-conf-file.c  class-conf-file.h \
	base.c      base.h      \
	lomo.c	lomo.h \
	playlist.c  playlist.h  \
	vogon.c     vogon.h   \
	fs.c        fs.h        \
	settings.c  settings.h  \
	player.c    player.h    \
	cover.c     cover.h     \
	g-ext-curl.c g-ext-curl.h \
	log.c log.h \
	eina-player-volume.c eina-player-volume.h \
	eina-player-seek.c eina-player-seek.h \
	eina-file-chooser-dialog.c eina-file-chooser-dialog.h \
	iface.c iface.h

eina_LDADD = \
	@PACKAGE_LIBS@

eina_LDFLAGS = -export-dynamic  -module -avoid-version \
	-static $(top_builddir)/gel/libgel.la \
	-static $(top_builddir)/libghub/libghub.la \
	--no-gc-sections

