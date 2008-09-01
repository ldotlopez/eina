## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE_NAME)\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

noinst_LTLIBRARIES = libgel.la

libgel_la_SOURCES =         \
	gel.c \
	gel-ui.c \
	gel-io-async-read-op.c \
	gel.h \
	gel-io.h \
	gel-ui.h \
	gel-io-async-read-op.h

libgel_la_LDFLAGS = -export-dynamic


