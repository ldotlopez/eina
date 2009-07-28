ntfydir = $(libdir)/$(PACKAGE)/ntfy

ntfy_LTLIBRARIES = libntfy.la

AM_CFLAGS = @PACKAGE_CFLAGS@ @NOTIFY_CFLAGS@
libntfy_la_LDFLAGS = @PACKAGE_LIBS@ @NOTIFY_LIBS@ -module -avoid-version
libntfy_la_SOURCES = ntfy.c ntfy.h

