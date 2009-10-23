SUBDIRS = ext

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

eina_LDADD = @PACKAGE_LIBS@ $(top_builddir)/eina/ext/.libs/libeinaext.a -lgel-1.0 -llomo-1.0


NOINST_H_FILES = \
	about.h    \
	dock.h     \
	player.h   \
	playlist.h \
	template.h \
	vogon.h 

INST_H_FILES = \
	art.h         \
	eina-obj.h    \
	eina-plugin.h \
	eina-stock.h  \
	fs.h          \
	lomo.h        \
	preferences.h \
	settings.h    \
	window.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/eina
header_DATA = $(INST_H_FILES)

# template.{c,h} are template files to create Eina components
eina_SOURCES = \
	about.c            \
	art.c              \
	cover.c            \
	dock.c             \
	eina-obj.c         \
	eina-plugin.c      \
	eina-stock.c       \
	fs.c               \
	log.c              \
	lomo.c             \
	main.c             \
	player.c           \
	playlist.c         \
	plugins.c          \
	preferences.c      \
	settings.c         \
	template.c         \
	vogon.c            \
	window.c           \
	$(NOINST_H_FILES)  \
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
AM_CPPFLAGS += @IGE_CFLAGS@
eina_LDADD += @IGE_LIBS@
endif

