## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = subdir-objects
EXTRA_DIST = \
	intltool-extract.in intltool-merge.in intltool-update.in \
	COPYRIGHT README AUTHORS INSTALL \
	gel/gel-app-marshallers.list     \
	lomo/lomo-marshallers.list       \
	eina/ext/eina-plugin-dialog.ui            \
	eina/ext/eina-plugin-properties.ui        \
	eina/ext/eina-preferences-dialog-marshallers.list \
	eina/dbus-marshallers.list

BUILT_SOURCES = \
	gel/gel-app-marshallers.h            \
	gel/gel-app-marshallers.c            \
	lomo/lomo-marshallers.c              \
	lomo/lomo-marshallers.h              \
	eina/ext/eina-plugin-dialog-ui.h     \
    eina/ext/eina-plugin-properties-ui.h \
    eina/ext/eina-preferences-dialog-marshallers.c \
    eina/ext/eina-preferences-dialog-marshallers.h \
	eina/dbus-marshallers.c \
	eina/dbus-marshallers.h

CLEANFILES = $(BUILT_SOURCES)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_PREFIX=\""$(prefix)"\" \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	@PACKAGE_CFLAGS@

PACKAGE_LIBS += $(INTLLIBS)

# Version X.Y must be specified
bin_PROGRAMS = eina/eina eina/vogon
lib_LTLIBRARIES = gel/libgel-1.0.la lomo/liblomo-1.0.la
noinst_LTLIBRARIES = eina/ext/libeinaext.la

#
# libgel
#
gel_libgel_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0 -export-dynamic 
gel_libgel_1_0_la_CFLAGS = -DGEL_COMPILATION

gel_includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/gel
gel_include_HEADERS = \
	gel/gel.h                   \
    gel/gel-misc.h              \
    gel/gel-string.h            \
    gel/gel-str-parser.h        \
    gel/gel-plugin.h            \
    gel/gel-app.h               \
    gel/gel-io-misc.h           \
    gel/gel-io-tree.h           \
    gel/gel-io-op-result.h      \
    gel/gel-io-ops.h            \
    gel/gel-io-recurse-tree.h   \
    gel/gel-io.h                \
    gel/gel-ui.h

gel_libgel_1_0_la_SOURCES = \
    gel/gel-misc.c              \
    gel/gel-string.c            \
    gel/gel-str-parser.c        \
    gel/gel-app-marshallers.c   \
	gel/gel-app-marshallers.h   \
    gel/gel-plugin.c            \
    gel/gel-app.c               \
    gel/gel-io-misc.c           \
    gel/gel-io-tree.c           \
    gel/gel-io-op-result.c      \
    gel/gel-io-ops.c            \
    gel/gel-io-recurse-tree.c   \
    gel/gel-ui.c

gel/gel-app-marshallers.h: gel/gel-app-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_app_marshal --header > $@

gel/gel-app-marshallers.c: gel/gel-app-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_app_marshal --header --body > $@

#
# liblomo
#
lomo_liblomo_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0
lomo_liblomo_1_0_la_CFLAGS = -DLIBLOMO_COMPILATION

lomo_includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/lomo
lomo_include_HEADERS = \
    lomo/lomo-player.h           \
    lomo/lomo-metadata-parser.h  \
    lomo/lomo-stream.h           \
    lomo/lomo-util.h

lomo_liblomo_1_0_la_SOURCES = \
	lomo/lomo-marshallers.c     \
	lomo/lomo-marshallers.h     \
    lomo/lomo-metadata-parser.c \
    lomo/lomo-metadata-parser.h \
    lomo/lomo-player.c          \
    lomo/lomo-playlist.c        \
    lomo/lomo-playlist.h        \
    lomo/lomo-stream.c          \
    lomo/lomo-util.c

lomo/lomo-marshallers.c: lomo/lomo-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=lomo_marshal --header --body > $@

lomo/lomo-marshallers.h: lomo/lomo-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=lomo_marshal --header > $@

#
# libeinaext
#
eina_ext_libeina_includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/ext
eina_ext_libeina_include_HEADERS = \
	eina/ext/curl-engine.h              \
    eina/ext/eina-conf.h                \
    eina/ext/eina-file-chooser-dialog.h \
    eina/ext/eina-plugin-dialog.h       \
    eina/ext/eina-plugin-properties.h   \
    eina/ext/eina-preferences-dialog.h  \
    eina/ext/eina-seek.h                \
    eina/ext/eina-stock.h               \
    eina/ext/eina-volume.h              \
    eina/ext/eina-window.h

eina_ext_libeinaext_la_SOURCES = \
    eina/ext/curl-engine.c              \
    eina/ext/eina-conf-file.c           \
    eina/ext/eina-file-chooser-dialog.c \
    eina/ext/eina-plugin-dialog.c       \
    eina/ext/eina-plugin-properties.c   \
    eina/ext/eina-preferences-dialog.c  \
    eina/ext/eina-preferences-dialog-marshallers.c  \
    eina/ext/eina-preferences-dialog-marshallers.h  \
    eina/ext/eina-seek.c                \
    eina/ext/eina-stock.c               \
    eina/ext/eina-volume.c              \
    eina/ext/eina-window.c

eina/ext/eina-preferences-dialog-marshallers.h: eina/ext/eina-preferences-dialog-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_preferences_dialog_marshal --header > $@

eina/ext/eina-preferences-dialog-marshallers.c: eina/ext/eina-preferences-dialog-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_preferences_dialog_marshal --header --body > $@

eina/ext/eina-plugin-dialog-ui.h: eina/ext/eina-plugin-dialog.ui
	tools/ui2h $< > $@ ui_xml

eina/ext/eina-plugin-properties-ui.h: eina/ext/eina-plugin-properties.ui
	tools/ui2h $< > $@ ui_xml

if USE_CLUTTER
eina_ext_libeinaext_la_SOURCES += eina/ext/eina-clutty.c
eina_ext_libeina_include_HEADERS += eina/ext/eina-clutty.h
endif

#
# eina
#
eina_eina_CFLAGS  = -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
eina_vogon_CFLAGS = -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 

eina_eina_LDFLAGS = -export-dynamic  -module -avoid-version \
	@PACKAGE_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	$(top_builddir)/eina/ext/.libs/libeinaext.a -lgel-1.0 -llomo-1.0

eina_vogon_LDFLAGS = -export-dynamic  -module -avoid-version \
	@PACKAGE_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	$(top_builddir)/eina/ext/.libs/libeinaext.a -lgel-1.0 -llomo-1.0 #LDDADD

eina_eina_includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
eina_eina_include_HEADERS = \
	eina/art.h         \
    eina/dbus.h        \
    eina/eina-obj.h    \
    eina/eina-plugin.h \
    eina/fs.h          \
    eina/lomo.h        \
    eina/preferences.h \
    eina/settings.h    \
    eina/window.h

# template.{c,h} are template files to create Eina components
eina_eina_SOURCES = \
    eina/about.c            \
    eina/about.h            \
    eina/art.c              \
    eina/cover.c            \
    eina/dock.c             \
    eina/dock.h             \
    eina/eina-obj.c         \
    eina/eina-plugin.c      \
    eina/fs.c               \
    eina/log.c              \
    eina/lomo.c             \
    eina/main.c             \
    eina/player.c           \
    eina/player.h           \
    eina/playlist.c         \
    eina/playlist.h         \
    eina/plugins.c          \
    eina/preferences.c      \
    eina/settings.c         \
    eina/template.c         \
    eina/template.h         \
    eina/vogon.c            \
    eina/vogon.h            \
    eina/window.c

eina_vogon_SOURCES = \
    eina/eina-obj.c   \
    eina/about.c      \
    eina/lomo.c       \
    eina/fs.c         \
    eina/settings.c   \
    eina/vogon.c      \
    eina/vogon-app.c

eina/dbus-marshallers.h: eina/dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header > $@

eina/dbus-marshallers.c: eina/dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header --body > $@

if BUILD_DBUS 
eina_eina_SOURCES += eina/dbus.c data/lomo-player-glue.h eina/dbus-marshallers.c eina/dbus-marshallers.h
eina_eina_CFLAGS += @DBUS_GLIB_CFLAGS@
eina_eina_LDFLAGS += @DBUS_GLIB_LIBS@ #LDDADD
endif

if BUILD_IGE
eina_eina_SOURCES += eina/eina-ige.c
eina_eina_CFLAGS  += @IGE_CFLAGS@
eina_eina_LDFLAGS += @IGE_LIBS@ #LDDADD
endif

#
# Subdirs
#

# po and docs are processed at the end
SUBDIRS = \
	data    \
	icons   \
	tools   \
	pixmaps \
	plugins \
	ui      \
	.       \
	docs    \
	po

