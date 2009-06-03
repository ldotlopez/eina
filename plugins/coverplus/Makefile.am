coverplusdir = $(pkglibdir)/coverplus
coverplus_LTLIBRARIES = libcoverplus.la

NOINST_H_FILES = banshee.h infolder.h

AM_CFLAGS = @PACKAGE_CFLAGS@
libcoverplus_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version

libcoverplus_la_SOURCES = \
	$(NOINST_H_FILES) \
	banshee.c         \
	iface.c           \
	infolder-sync.c

