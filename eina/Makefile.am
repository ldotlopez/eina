INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ @CURL_CXXFLAGS@

bin_PROGRAMS = eina

eina_LDFLAGS = -export-dynamic  -module -avoid-version \
	-static $(top_builddir)/gel/libgel.la

eina_LDADD = @PACKAGE_LIBS@ @CURL_LIBS@

NOINST_H_FILES = \
	class-base.h               \
	class-conf-file.h          \
	cover.h                    \
	eina-file-chooser-dialog.h \
	eina-player-seek.h         \
	eina-player-volume.h       \
	ether.h                    \
	log.h                      \
	lomo.h                     \
	player.h                   \
	playlist.h                 \
	template.h                 \
	vogon.h

INST_H_FILES = \
	base.h                     \
	iface.h                    \
	fs.h                       \
	settings.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/eina
header_DATA = $(INST_H_FILES)

# ether.{c,h} are template files to create GObjects
# template.{c,h} are template files to create Eina components
eina_SOURCES = \
	base.c                     \
	class-base.c               \
	class-conf-file.c          \
	cover.c                    \
	eina-file-chooser-dialog.c \
	eina-player-seek.c         \
	eina-player-volume.c       \
	ether.c                    \
	fs.c                       \
	iface.c                    \
	log.c                      \
	lomo.c                     \
	main.c                     \
	player.c                   \
	playlist.c                 \
	settings.c                 \
	template.c                 \
	vogon.c                    \
	$(NOINST_H_FILES)          \
	$(INST_H_FILES)

