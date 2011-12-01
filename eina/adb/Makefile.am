include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/adb
plugin_LTLIBRARIES = libadb.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/adb
include_HEADERS = \
	eina-adb-plugin.h \
	eina-adb.h        \
	eina-adb-result.h \
	eina-adb-lomo.h

libadb_la_CFLAGS  = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
libadb_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
libadb_la_SOURCES = \
	$(include_HEADERS) \
	eina-adb-plugin.c  \
	eina-adb.c         \
	eina-adb-result.c  \
	eina-adb-lomo.c    \
	eina-adb-upgrade.c \
	register.c         \
	register.h

plugin_DATA = adb.plugin
EXTRA_DIST += $(plugin_DATA)


-include $(top_srcdir)/git.mk
