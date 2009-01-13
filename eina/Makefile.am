INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DEINA_COMPILATION \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = eina

eina_LDFLAGS = -export-dynamic  -module -avoid-version \
	-L$(top_srcdir)/gel/.libs \
	-L$(top_srcdir)/lomo/.libs

#-static $(top_builddir)/gel/libgel.la \
#-static $(top_builddir)/lomo/liblomo-1.0.la

eina_LDADD = @PACKAGE_LIBS@ -lgel -llomo-1.0

NOINST_H_FILES = \
	dock.h                     \
	eina-file-chooser-dialog.h \
	eina-seek.h                \
	eina-volume.h              \
	ether.h                    \
	lomo.h                     \
	player.h                   \
	playlist.h                 \
	playlist2.h                \
	eina-plugin.h              \
	plugins2.h                 \
	template.h                 \
	vogon.h

INST_H_FILES = \
	artwork.h                  \
	base.h                     \
	eina-obj.h                 \
	class-conf-file.h          \
	eina-artwork.h             \
	eina-preferences-dialog.h  \
	fs.h                       \
	preferences.h              \
	settings.h                 \
	settings2.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/eina
header_DATA = $(INST_H_FILES)

# ether.{c,h} are template files to create GObjects
# template.{c,h} are template files to create Eina components
eina_SOURCES = \
	artwork.c                  \
	base.c                     \
	eina-obj.c                 \
	class-conf-file.c          \
	dock.c                     \
	eina-artwork.c             \
	eina-file-chooser-dialog.c \
	eina-preferences-dialog.c  \
	eina-seek.c                \
	eina-volume.c              \
	ether.c                    \
	fs.c                       \
	log.c                      \
	lomo.c                     \
	main.c                     \
	player.c                   \
	playlist.c                 \
	playlist2.c                \
	eina-plugin.c              \
	plugins2.c                 \
	preferences.c              \
	settings.c                 \
	settings2.c                \
	template.c                 \
	vogon.c                    \
	vogon2.c                   \
	$(NOINST_H_FILES)          \
	$(INST_H_FILES)

