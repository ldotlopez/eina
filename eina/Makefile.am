include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = ext

bin_PROGRAMS = eina vogon
eina_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
eina_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
eina_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0

vogon_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
vogon_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
vogon_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0


includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
include_HEADERS = \
	about.h       \
	art.h         \
	dbus.h        \
	dock.h        \
	eina-obj.h    \
	eina-plugin.h \
	fs.h          \
	lomo.h        \
	player.h      \
	playlist.h    \
	preferences.h \
	settings.h    \
	vogon.h       \
	window.h

eina_SOURCES = \
	about.c            \
	art.c              \
	dock.c             \
	eina-obj.c         \
	eina-plugin.c      \
	fs.c               \
	log.c              \
	lomo.c             \
	main.c             \
	player.c           \
	playlist.c         \
	plugins.c          \
	preferences.c      \
	settings.c         \
	vogon.c            \
	window.c

vogon_SOURCES = \
	art.c \
	eina-obj.c \
	fs.c \
	lomo.c \
	settings.c \
	vogon-app.c

EXTRA_DIST += dbus-marshallers.list

if BUILD_DBUS
eina_SOURCES += \
	dbus.c \
	dbus.h \
	dbus-marshallers.c \
	dbus-marshallers.h \
	lomo-player-dbus-glue.h 

eina_CFLAGS += @DBUS_GLIB_CFLAGS@
eina_LDFLAGS += @DBUS_GLIB_LIBS@

lomo-player-dbus-glue.h: lomo-player-dbus-glue.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=lomo_player $< > $@

dbus-marshallers.h: dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header > $@

dbus-marshallers.c: dbus-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_dbus_marshal --header --body > $@

BUILT_SOURCES += \
	dbus-marshallers.c \
	dbus-marshallers.h
	lomo-player-dbus-glue.h 
endif

if BUILD_IGE
eina_SOURCES += eina-ige.c
eina_CFLAGS += @IGE_CFLAGS@
eina_LDFLAGS += @IGE_LIBS@
endif

if BUILD_UNIQUE
eina_CFLAGS += @UNIQUE_CFLAGS@
eina_LDFLAGS += @UNIQUE_LIBS@

vogon_CFLAGS += @UNIQUE_CFLAGS@
vogon_LDFLAGS += @UNIQUE_LIBS@
endif

