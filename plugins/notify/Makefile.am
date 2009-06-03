ntfydir = $(libdir)/$(PACKAGE)/notify

ntfy_LTLIBRARIES = libnotify.la

AM_CFLAGS = @PACKAGE_CFLAGS@ @NOTIFY_CFLAGS@
libnotify_la_LDFLAGS = @PACKAGE_LIBS@ @NOTIFY_LIBS@ -module -avoid-version
libnotify_la_SOURCES = ntfy.c 

