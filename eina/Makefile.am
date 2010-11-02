include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = ext   \
	application \
	art         \
	dbus        \
	dock        \
	lastfm      \
	lomo        \
	player      \
	playlist    \
	preferences \
	statusicon

if BUILD_ADB
SUBDIRS +=   \
	adb      \
	muine
endif
#	recently

if BUILD_NTFY
SUBDIRS += ntfy
endif

DIST_SUBDIRS = \
	$(SUBDIRS) \
	adb        \
	muine      \
	ntfy       \
	recently

bin_PROGRAMS = eina
eina_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
eina_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
eina_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
include_HEADERS = \
	eina-plugin.h

eina_SOURCES = \
	eina.c

