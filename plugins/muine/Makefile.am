muinedir = $(pkglibdir)/muine
muine_LTLIBRARIES = libmuine.la

headerdir = $(includedir)/eina-@EINA_API_VERSION@/plugins/muine
header_DATA = $(INST_H_FILES)

AM_CFLAGS         = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
libmuine_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version

libmuine_la_SOURCES =   \
	muine.c

