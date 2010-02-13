muinedir = $(pkglibdir)/muine
muine_LTLIBRARIES = libmuine.la
muine_DATA = muine.ui

AM_CFLAGS         = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
libmuine_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version

EXTRA_DIST = $(muine_DATA)

libmuine_la_SOURCES =   \
	muine.c

