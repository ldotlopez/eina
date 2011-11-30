include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = ext   \
	dbus          \
	dock          \
	fieshta       \
	lastfm        \
	lomo          \
	mpris         \
	player        \
	playlist      \
	preferences   \
	pythonconsole \
	pythonhello   \
	status-icon

if BUILD_ADB
SUBDIRS +=   \
	adb   \
	muine \
	stars
endif

if BUILD_CLUTTY
SUBDIRS += clutty
endif

if BUILD_NTFY
SUBDIRS += ntfy
endif

bin_PROGRAMS = eina
eina_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
eina_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
eina_LDFLAGS = \
	@EINA_LIBS@

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
include_HEADERS =


eina_SOURCES = \
	eina.c

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = \
	--warn-all \
	--add-include-path=$(top_builddir)/lomo \
	--add-include-path=$(top_builddir)/gel
INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(top_builddir)/lomo \
	--includedir=$(top_builddir)/gel

if HAVE_INTROSPECTION
Eina-0.12.gir: eina
Eina_0_12_gir_INCLUDES = Lomo-2.0 GelUI-2.0 Peas-1.0
Eina_0_12_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)
Eina_0_12_gir_PROGRAM = $(top_builddir)/eina/eina
Eina_0_12_gir_LDADD = \
	$(top_builddir)/lomo/liblomo.la \
	$(top_builddir)/dock/libdock.la \
	$(top_builddir)/playlist/libplaylist.la
Eina_0_12_gir_FILES = $(eina_SOURCES) $(include_HEADERS) \
	$(top_srcdir)/eina/ext/eina-application.h  \
	$(top_srcdir)/eina/ext/eina-application.c  \
	$(top_srcdir)/eina/ext/eina-activatable.h  \
	$(top_srcdir)/eina/ext/eina-activatable.c  \
	$(top_srcdir)/eina/ext/eina-window.h       \
	$(top_srcdir)/eina/ext/eina-window.c       \
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

INTROSPECTION_GIRS += Eina-0.12.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

-include $(top_srcdir)/git.mk
