AUTOMAKE_OPTIONS = subdir-objects

#
# Subdirs
#

# Some non-code subdirs follow a recursive schema for convenience
SUBDIRS = \
	.       \
	docs    \
	icons   \
	po

EXTRA_DIST = \
	run-uninstalled.sh \
	intltool-extract.in intltool-merge.in intltool-update.in \
	AUTHORS BUGS COPYING INSTALL NEWS README

BUILT_SOURCES = 
CLEANFILES =
DISTCLEANFILES =

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_PREFIX=\""$(prefix)"\" \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)/$(PACKAGE)"\" \
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
    gel/gel-job-queue.h         \
    gel/gel-plugin.h            \
    gel/gel-plugin-info.h       \
    gel/gel-app.h               \
    gel/gel-io-misc.h           \
    gel/gel-io-tree.h           \
    gel/gel-io-op-result.h      \
    gel/gel-io-ops.h            \
    gel/gel-io-recurse-tree.h   \
	gel/gel-io-scanner.h        \
    gel/gel-io.h                \
    gel/gel-ui.h

gel_libgel_1_0_la_SOURCES = \
    gel/gel-misc.c              \
    gel/gel-string.c            \
    gel/gel-str-parser.c        \
    gel/gel-job-queue.c         \
    gel/gel-app-marshallers.c   \
	gel/gel-app-marshallers.h   \
    gel/gel-plugin.c            \
    gel/gel-plugin-info.c       \
    gel/gel-app.c               \
	gel/gel-io-marshallers.c    \
    gel/gel-io-misc.c           \
    gel/gel-io-tree.c           \
    gel/gel-io-op-result.c      \
    gel/gel-io-ops.c            \
    gel/gel-io-recurse-tree.c   \
	gel/gel-io-scanner.c        \
    gel/gel-ui.c

EXTRA_DIST += \
	gel/gel-app-marshallers.list \
	gel/gel-io-marshallers.list

BUILT_SOURCES += \
	gel/gel-app-marshallers.h \
	gel/gel-app-marshallers.c \
	gel/gel-io-marshallers.h  \
	gel/gel-io-marshallers.c

gel/gel-app-marshallers.h: gel/gel-app-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_app_marshal --header > $@

gel/gel-app-marshallers.c: gel/gel-app-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_app_marshal --header --body > $@

gel/gel-io-marshallers.h: gel/gel-io-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_io_marshal --header > $@

gel/gel-io-marshallers.c: gel/gel-io-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=gel_io_marshal --header --body > $@

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

EXTRA_DIST += \
	lomo/lomo-marshallers.list

BUILT_SOURCES += \
	lomo/lomo-marshallers.c \
	lomo/lomo-marshallers.h

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
    eina/ext/eina-cover.h               \
    eina/ext/eina-cover-image.h         \
    eina/ext/eina-file-chooser-dialog.h \
    eina/ext/eina-file-utils.h          \
    eina/ext/eina-plugin-dialog.h       \
    eina/ext/eina-preferences-dialog.h  \
    eina/ext/eina-preferences-tab.h     \
    eina/ext/eina-seek.h                \
    eina/ext/eina-stock.h               \
    eina/ext/eina-window.h

eina_ext_libeinaext_la_SOURCES = \
    eina/ext/curl-engine.c              \
    eina/ext/eina-conf-file.c           \
    eina/ext/eina-cover.c               \
    eina/ext/eina-cover-image.c         \
    eina/ext/eina-file-chooser-dialog.c \
    eina/ext/eina-file-utils.c          \
    eina/ext/eina-plugin-dialog.c       \
    eina/ext/eina-preferences-dialog.c  \
    eina/ext/eina-preferences-tab.c     \
    eina/ext/eina-seek.c                \
    eina/ext/eina-stock.c               \
    eina/ext/eina-window.c

EXTRA_DIST += \
	eina/ext/eina-plugin-dialog.ui            \
	eina/ext/eina-plugin-properties.ui

BUILT_SOURCES += \
	eina/ext/eina-cover-image-mask.h               \
	eina/ext/eina-plugin-dialog-ui.h               \
    eina/ext/eina-plugin-properties-ui.h

eina/ext/eina-cover-image-mask.h: pixmaps/cover-mask.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=__cover_mask $<  > $@

eina/ext/eina-plugin-dialog-ui.h: eina/ext/eina-plugin-dialog.ui
	$(top_srcdir)/tools/ui2h $< ui_xml > $@

eina/ext/eina-plugin-properties-ui.h: eina/ext/eina-plugin-properties.ui
	$(top_srcdir)/tools/ui2h $< ui_xml > $@


#
# eina
#
eina_eina_CFLAGS  = $(AM_CPPFLAGS) -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
eina_vogon_CFLAGS = $(AM_CPPFLAGS) -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 

eina_eina_LDFLAGS = -export-dynamic  -module -avoid-version \
	@PACKAGE_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0
eina_eina_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la

eina_vogon_LDFLAGS = -export-dynamic  -module -avoid-version \
	@PACKAGE_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0
eina_vogon_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la

eina_eina_includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
eina_eina_include_HEADERS = \
	eina/art.h         \
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

if BUILD_DBUS 
eina_eina_SOURCES += \
	eina/dbus.c \
	eina/dbus.h \
	eina/lomo-player-dbus-glue.h \
	eina/dbus-marshallers.c \
	eina/dbus-marshallers.h

eina_eina_CFLAGS  += @DBUS_GLIB_CFLAGS@
eina_eina_LDFLAGS += @DBUS_GLIB_LIBS@

eina/lomo-player-dbus-glue.h: eina/lomo-player-dbus-glue.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=lomo_player $< > $@

eina/dbus-marshallers.h: eina/dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header > $@

eina/dbus-marshallers.c: eina/dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header --body > $@

BUILT_SOURCES += \
	eina/lomo-player-dbus-glue.h \
	eina/dbus-marshallers.c \
	eina/dbus-marshallers.h
endif

if BUILD_IGE
eina_eina_SOURCES += eina/eina-ige.c
eina_eina_CFLAGS  += @IGE_CFLAGS@
eina_eina_LDFLAGS += @IGE_LIBS@
endif

EXTRA_DIST += \
	eina/dbus-marshallers.list \
	eina/lomo-player-dbus-glue.xml

#
# ui
#
uidir = $(pkgdatadir)/ui
ui_DATA = \
	ui/dock.ui \
    ui/log.ui \
    ui/player-preferences.ui \
    ui/player.ui \
    ui/playlist.ui \
    ui/plugins.ui

EXTRA_DIST += $(ui_DATA)

#
# pixmaps
#
pixmapsdir = $(pkgdatadir)/pixmaps
pixmaps_DATA =  \
	pixmaps/cover-default.png      \
	pixmaps/cover-loading.png      \
	pixmaps/cover-mask.png         \
	pixmaps/eina.svg               \
	pixmaps/loading-spin-16x16.gif \
	pixmaps/plugin.png
EXTRA_DIST += $(pixmaps_DATA)

#
# data
#

# pkgconfig files
data_pkgconfigdir = $(libdir)/pkgconfig
data_pkgconfig_DATA = data/eina-@EINA_API_VERSION@.pc

# desktop files
data_applicationsdir = $(datadir)/applications
data_applications_DATA = data/eina.desktop
@INTLTOOL_DESKTOP_RULE@

# schema
gsettings_SCHEMAS = data/net.sourceforge.eina.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

DISTCLEANFILES += \
	$(gsettings_SCHEMAS)
CLEANFILES += \
	$(gsettings_SCHEMAS)
EXTRA_DIST += \
	tools/ui2h \
	data/net.sourceforge.eina.gschema.xml.in.in
MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

if BUILD_PLUGINS

#
# Adb plugin
#
plugins_adbdir = $(pkglibdir)/adb
plugins_adb_includedir = $(includedir)/@PACKAGE@-@EINA_API_VERSION@/plugins/adb
plugins_adb_include_HEADERS = plugins/adb/adb.h
plugins_adb_DATA = plugins/adb/adb.png 

if BUILD_ADB
plugins_adb_LTLIBRARIES = plugins/adb/libadb.la
plugins_adb_libadb_la_CFLAGS  = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@ 
plugins_adb_libadb_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
plugins_adb_libadb_la_SOURCES = \
	plugins/adb/eina-adb.c \
	plugins/adb/eina-adb.h \
	plugins/adb/eina-adb-upgrade.c \
	plugins/adb/eina-adb-lomo.c \
	plugins/adb/eina-adb-lomo.h \
	plugins/adb/register.c \
	plugins/adb/register.h \
	plugins/adb/adb.c      \
	plugins/adb/adb.h
endif

EXTRA_DIST += \
	$(plugins_adb_DATA) \
	$(plugins_adb_include_HEADERS) \
	plugins/adb/adb.svg

#
# callhome plugin
#
if BUILD_CALLHOME
plugins_callhomedir = $(pkglibdir)/callhome
plugins_callhome_LTLIBRARIES = plugins/callhome/libcallhome.la

plugins_callhome_libcallhome_la_CFLAGS  = @PACKAGE_CFLAGS@ @UUID_CFLAGS@
plugins_callhome_libcallhome_la_LDFLAGS = @PACKAGE_LIBS@ @UUID_LIBS@ -module -avoid-version
plugins_callhome_libcallhome_la_SOURCES = plugins/callhome/callhome.c
endif

#
# clutty plugin
#
if BUILD_CLUTTY
plugins_cluttydir = $(pkglibdir)/clutty
plugins_clutty_LTLIBRARIES = plugins/clutty/libclutty.la

plugins_clutty_libclutty_la_CFLAGS  = @PACKAGE_CFLAGS@ @CLUTTER_GTK_CFLAGS@
plugins_clutty_libclutty_la_LDFLAGS = @PACKAGE_LIBS@ @CLUTTER_GTK_LIBS@ -module -avoid-version
plugins_clutty_libclutty_la_SOURCES =   \
	plugins/clutty/clutty.c             \
	plugins/clutty/eina-cover-clutter.c \
	plugins/clutty/eina-cover-clutter.h

endif

#
# coverplus plugin
#
plugins_coverplusdir = $(pkglibdir)/coverplus
plugins_coverplus_LTLIBRARIES = plugins/coverplus/libcoverplus.la

plugins_coverplus_libcoverplus_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
plugins_coverplus_libcoverplus_la_SOURCES = \
	plugins/coverplus/banshee.c       \
	plugins/coverplus/banshee.h       \
	plugins/coverplus/infolder-sync.c \
	plugins/coverplus/infolder.h      \
	plugins/coverplus/coverplus.c

#
# fieshta plugin
#
if BUILD_EXPERIMENTAL

plugins_fieshtadir = $(pkglibdir)/fieshta
plugins_fieshta_LTLIBRARIES = plugins/fieshta/libfieshta.la

plugins_fieshta_libfieshta_la_CFLAGS  = @PACKAGE_LIBS@
plugins_fieshta_libfieshta_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
plugins_fieshta_libfieshta_la_SOURCES = \
	plugins/fieshta/fieshta.c \
	plugins/fieshta/fieshta.h

if USE_CLUTTER_GTK
plugins_fieshta_libfieshta_la_SOURCES += \
	plugins/fieshta/fieshta-stage.c  \
	plugins/fieshta/fieshta-stage.h  \
	plugins/fieshta/fieshta-stream.c \
	plugins/fieshta/fieshta-stream.h

plugins_fieshta_libfieshta_la_CFLAGS  += @CLUTTER_GTK_CFLAGS@
plugins_fieshta_libfieshta_la_LDFLAGS += @CLUTTER_GTK_LIBS@
endif
endif

#
# lastfm plugin
#
plugins_lastfmdir = $(pkglibdir)/lastfm

plugins_lastfm_LTLIBRARIES = plugins/lastfm/liblastfm.la
plugins_lastfm_DATA = \
	plugins/lastfm/lastfm.png \
	plugins/lastfm/lastfm.ui  \
	plugins/lastfm/logo.gif

#if USE_LASTFM_WEBVIEW
#liblastfm_la_LDFLAGS += @WEBKITGTK_LIBS@
#liblastfm_la_SOURCES += webview.c webview.h
#endif

plugins_lastfm_liblastfm_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
plugins_lastfm_liblastfm_la_SOURCES = \
	plugins/lastfm/lastfm.c  \
	plugins/lastfm/lastfm.h  \
	plugins/lastfm/submit.c  \
	plugins/lastfm/submit.h  \
	plugins/lastfm/artwork.c \
	plugins/lastfm/artwork.h

plugins_lastfm_lastfmsubmitddir = $(plugins_lastfmdir)/lastfmsubmitd
plugins_lastfm_lastfmsubmitd_DATA = \
	plugins/lastfm/lastfmsubmitd/lastfmsubmitd \
	plugins/lastfm/lastfmsubmitd/COPYRIGHT \
	plugins/lastfm/lastfmsubmitd/info.txt
plugins_lastfm_lastfmsubmitdlibdir = $(plugins_lastfm_lastfmsubmitddir)/lastfm
plugins_lastfm_lastfmsubmitdlib_DATA = \
	plugins/lastfm/lastfmsubmitd/lastfm/__init__.py   \
	plugins/lastfm/lastfmsubmitd/lastfm/marshaller.py \
	plugins/lastfm/lastfmsubmitd/lastfm/config.py     \
	plugins/lastfm/lastfmsubmitd/lastfm/client.py

EXTRA_DIST += \
	$(plugins_lastfm_DATA) \
	$(plugins_lastfm_lastfmsubmitd_DATA) \
	$(plugins_lastfm_lastfmsubmitdlib_DATA)

#
# muine plugin
#
plugins_muinedir = $(pkglibdir)/muine
plugins_muine_DATA = plugins/muine/muine.ui

if BUILD_ADB
plugins_muine_LTLIBRARIES = plugins/muine/libmuine.la

plugins_muine_libmuine_la_CFLAGS  = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
plugins_muine_libmuine_la_LDFLAGS = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@ -module -avoid-version
plugins_muine_libmuine_la_SOURCES = plugins/muine/muine.c
endif

EXTRA_DIST += $(plugins_muine_DATA)

#
# ntfy plugin
#
plugins_ntfydir = $(pkglibdir)/ntfy
plugins_ntfy_DATA = plugins/ntfy/ntfy.png

if BUILD_NTFY
plugins_ntfy_LTLIBRARIES = plugins/ntfy/libntfy.la

plugins_ntfy_libntfy_la_CFLAGS  = @PACKAGE_CFLAGS@ @NOTIFY_CFLAGS@
plugins_ntfy_libntfy_la_LDFLAGS = @PACKAGE_LIBS@ @NOTIFY_LIBS@ -module -avoid-version
plugins_ntfy_libntfy_la_SOURCES = \
	plugins/ntfy/ntfy.c \
	plugins/ntfy/ntfy.h
endif

EXTRA_DIST += \
	$(plugins_ntfy_DATA) \
	plugins/ntfy/ntfy.svg

#
# recently plugin
#
plugins_recentlydir = $(pkglibdir)/recently
plugins_recently_DATA = \
	plugins/recently/dock.ui \
	plugins/recently/recently.png

if BUILD_EXPERIMENTAL
if BUILD_ADB
plugins_recently_LTLIBRARIES   = plugins/recently/librecently.la
plugins_recently_librecently_la_CFLAGS  = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
plugins_recently_librecently_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
plugins_recently_librecently_la_SOURCES = plugins/recently/recently.c

endif
endif

EXTRA_DIST += \
	$(plugins_recently_DATA) \
	plugins/recently/recently.svg

endif

# Final CLEANFILES
CLEANFILES += $(BUILT_SOURCES)

