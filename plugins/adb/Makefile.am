include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/adb
plugin_LTLIBRARIES = libadb.la
plugin_DATA = adb.png
EXTRA_DIST += $(plugin_DATA) \
	adb.svg

libadb_la_CFLAGS = @EINA_CFLAGS@ @SQLITE3_CFLAGS@
libadb_la_LDFLAGS = @EINA_LIBS@ @SQLITE3_LIBS@ -module -avoid-version

INST_H_FILES = adb.h
NOINST_H_FILES = register.h

headerdir = $(includedir)/@PACKAGE@-@EINA_API_VERSION@/plugins/adb
header_DATA = $(INST_H_FILES)

libadb_la_SOURCES = \
	$(INST_H_FILES)   \
	$(NOINST_H_FILES) \
	adb.c             \
	iface.c           \
	register.c



