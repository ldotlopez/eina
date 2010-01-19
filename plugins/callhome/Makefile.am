callhomedir = $(pkglibdir)/callhome
callhome_LTLIBRARIES = libcallhome.la

AM_CFLAGS = @PACKAGE_CFLAGS@ @UUID_CFLAGS@
libcallhome_la_LDFLAGS = @PACKAGE_LIBS@ @UUID_LIBS@ -module -avoid-version

libcallhome_la_SOURCES = callhome.c

