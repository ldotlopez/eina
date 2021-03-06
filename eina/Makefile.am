include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = core    \
	data          \
	dbus          \
	dock          \
	fieshta       \
	lastfm        \
	lomo          \
	lyrics        \
	mpris         \
	player        \
	playlist      \
	preferences   \
	pythonconsole \
	pythonhello   \
	status-icon

if BUILD_ADB
SUBDIRS += \
	adb      \
	importer \
	muine    \
	stars
endif

if BUILD_HIPSTER
SUBDIRS += \
	hipster
endif

if BUILD_CLUTTY
SUBDIRS += clutty
endif

if BUILD_NTFY
SUBDIRS += ntfy
endif

if BUILD_GTKMAC
SUBDIRS += osx
endif

bin_PROGRAMS = eina
eina_LDADD = \
	$(top_builddir)/eina/core/.libs/libeina-core.la

eina_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
eina_LDFLAGS = \
	@EINA_LIBS@

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
include_HEADERS =

eina_SOURCES = \
	resources.c \
	eina.c

eina_gresources_xml = resources.xml
eina_gresources_name = eina_core
include $(top_srcdir)/build/Makefile.am.gresources

include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = \
	--warn-all \
	--add-include-path=$(top_srcdir)/lomo \
	--add-include-path=$(top_srcdir)/gel
INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(top_builddir)/lomo \
	--includedir=$(top_builddir)/gel

Eina-0.16.gir: eina
Eina_0_16_gir_INCLUDES = Lomo-2.0 GelUI-2.0 Peas-1.0
Eina_0_16_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Eina_0_16_gir_PROGRAM = $(top_builddir)/eina/eina

#
# Introspection
#
Eina-0.14.gir: eina
Eina_0_14_gir_INCLUDES = Lomo-2.0 GelUI-2.0 Peas-1.0
Eina_0_14_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Eina_0_14_gir_PROGRAM = $(top_builddir)/eina/eina

# Not needed??
#Eina_0_16_gir_LDADD = \
#	$(top_builddir)/lomo/liblomo.la \
#	$(top_builddir)/dock/libdock.la \
#	$(top_builddir)/playlist/libplaylist.la
#   ...
Eina_0_16_gir_FILES = $(eina_SOURCES) $(include_HEADERS) \
	$(top_srcdir)/eina/core/eina-application.h  \
	$(top_srcdir)/eina/core/eina-application.c  \
	$(top_srcdir)/eina/core/eina-activatable.h  \
	$(top_srcdir)/eina/core/eina-activatable.c  \
	$(top_srcdir)/eina/core/eina-window.h       \
	$(top_srcdir)/eina/core/eina-window.c       \
	\
	$(top_srcdir)/eina/lomo/eina-lomo-plugin.h \
	$(top_srcdir)/eina/lomo/eina-lomo-plugin.c \
	\
	$(top_srcdir)/eina/dock/eina-dock-plugin.h \
	$(top_srcdir)/eina/dock/eina-dock-plugin.c \
	$(top_srcdir)/eina/dock/eina-dock.h \
	$(top_srcdir)/eina/dock/eina-dock.c \
	$(top_srcdir)/eina/dock/eina-dock-tab.h \
	$(top_srcdir)/eina/dock/eina-dock-tab.c \
	\
	$(top_srcdir)/eina/playlist/eina-playlist-plugin.h \
	$(top_srcdir)/eina/playlist/eina-playlist-plugin.c \
	$(top_srcdir)/eina/playlist/eina-playlist.h \
	$(top_srcdir)/eina/playlist/eina-playlist.c \
	\
	$(top_srcdir)/eina/player/eina-player-plugin.h \
	$(top_srcdir)/eina/player/eina-player-plugin.c \
	$(top_srcdir)/eina/player/eina-player.h \
	$(top_srcdir)/eina/player/eina-player.c \
	$(top_srcdir)/eina/player/eina-cover.c  \
	$(top_srcdir)/eina/player/eina-cover.h

if BUILD_ADB
Eina_0_16_gir_FILES += \
	$(top_srcdir)/eina/adb/eina-adb-plugin.c  \
	$(top_srcdir)/eina/adb/eina-adb-plugin.h  \
	$(top_srcdir)/eina/adb/eina-adb.h         \
	$(top_srcdir)/eina/adb/eina-adb.c         \
	$(top_srcdir)/eina/adb/eina-adb-result.h  \
	$(top_srcdir)/eina/adb/eina-adb-result.c  \
	$(top_srcdir)/eina/adb/eina-adb-lomo.c    \
	$(top_srcdir)/eina/adb/eina-adb-lomo.h    \
	$(top_srcdir)/eina/adb/eina-adb-upgrade.c
endif

INTROSPECTION_GIRS += Eina-0.16.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

#
# Misc
#
CLEANFILES += $(gir_DATA) $(typelib_DATA)
-include $(top_srcdir)/git.mk
