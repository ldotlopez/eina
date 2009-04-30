recentlydir = $(libdir)/$(PACKAGE)/recently
recently_DATA = dock.ui

recently_LTLIBRARIES   = librecently.la

AM_CFLAGS               = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
librecently_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
librecently_la_SOURCES = recently.c

BUILDSOURCES = dock.ui
EXTRA_DIST = dock.glade
CLEANSOURCES = dock.ui

dock.ui: dock.glade
	$(GTK_BUILDER_CONVERT) dock.glade dock.ui
