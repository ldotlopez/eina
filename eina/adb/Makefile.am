include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/adb
plugin_LTLIBRARIES = libadb.la
libadb_la_CFLAGS  = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
libadb_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@
libadb_la_SOURCES = \
	adb.c              \
	adb.h              \
	eina-adb.c         \
	eina-adb.h         \
	eina-adb-lomo.c    \
	eina-adb-lomo.h    \
	eina-adb-upgrade.c \
	register.c         \
	register.h

plugin_DATA = adb.ini

EXTRA_DIST += adb.ini

