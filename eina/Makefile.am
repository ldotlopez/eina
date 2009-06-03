AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	-DEINA_COMPILATION \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = eina

eina_LDFLAGS = -export-dynamic  -module -avoid-version \
	-L$(top_srcdir)/gel/.libs \
	-L$(top_srcdir)/lomo/.libs

# -static $(top_builddir)/gel/libgel.la 
# -static $(top_builddir)/lomo/liblomo-1.0.la

eina_LDADD = @PACKAGE_LIBS@ -lgel-1.0 -llomo-1.0 

NOINST_H_FILES = \
	dock.h                     \
	eina-file-chooser-dialog.h \
	eina-seek.h                \
	eina-volume.h              \
	player.h                   \
	playlist.h                 \
	template.h                 \
	vogon.h

INST_H_FILES = \
	art.h \
	curl-engine.h              \
	eina-lomo.h                \
	eina-obj.h                 \
	eina-conf.h                \
	eina-preferences-dialog.h  \
	eina-plugin.h              \
	eina-stock.h               \
	fs.h                       \
	preferences.h              \
	eina-settings.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/eina
header_DATA = $(INST_H_FILES)

# template.{c,h} are template files to create Eina components
eina_SOURCES = \
	art.c \
	eina-obj.c                 \
	eina-conf-file.c           \
	curl-engine.c              \
	dock.c                     \
	eina-file-chooser-dialog.c \
	eina-preferences-dialog.c  \
	eina-seek.c                \
	eina-volume.c              \
	fs.c                       \
	log.c                      \
	eina-lomo.c                \
	main.c                     \
	player.c                   \
	playlist.c                 \
	eina-plugin.c              \
	preferences.c              \
	eina-settings.c            \
	eina-stock.c               \
	template.c                 \
	vogon.c                    \
	plugins.c                  \
	$(NOINST_H_FILES)          \
	$(INST_H_FILES)

BUILT_SOURCES = dbus-marshallers.c dbus-marshallers.h
CLEAN_FILES = dbus-marshallers.c dbus-marshallers.h
EXTRA_DIST = dbus-marshallers.list

dbus-marshallers.h: dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header > $@

dbus-marshallers.c: dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header --body > $@

if USE_DBUS_GLIB
eina_SOURCES += dbus.c dbus-marshallers.c
NOINST_H_FILES += dbus-marshallers.h
endif

if USE_IGE
eina_SOURCES += eina-ige.c
eina_LDADD += @IGE_LIBS@
AM_CPPFLAGS += @IGE_CFLAGS@
endif

