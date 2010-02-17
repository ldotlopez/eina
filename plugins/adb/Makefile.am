adbdir = $(pkglibdir)/adb
adb_LTLIBRARIES = libadb.la

adb_DATA = adb.png
EXTRA_DIST = $(adb_DATA) adb.svg

INST_H_FILES = adb.h
NOINST_H_FILES = register.h

headerdir = $(includedir)/@PACKAGE@-@EINA_API_VERSION@/plugins/adb
header_DATA = $(INST_H_FILES)

AM_CFLAGS         = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
libadb_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version

libadb_la_SOURCES =   \
	$(INST_H_FILES)   \
	$(NOINST_H_FILES) \
	register.c        \
	adb.c             \
	iface.c

