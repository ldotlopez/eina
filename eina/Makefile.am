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
	class-conf-file.h          \
	eina-file-chooser-dialog.h \
	eina-seek.h                \
	eina-volume.h              \
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
	eina-cover.h               \
	fs.h                       \
	settings.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/eina
header_DATA = $(INST_H_FILES)

# ether.{c,h} are template files to create GObjects
# template.{c,h} are template files to create Eina components
eina_SOURCES = \
	base.c                     \
	class-conf-file.c          \
	eina-cover.c               \
	eina-file-chooser-dialog.c \
	eina-seek.c                \
	eina-volume.c              \
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

