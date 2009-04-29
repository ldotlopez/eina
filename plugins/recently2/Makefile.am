recently2dir = $(libdir)/$(PACKAGE)/recently2
recently2_DATA = dock.ui

recently2_LTLIBRARIES   = librecently2.la

AM_CFLAGS               = @PACKAGE_CFLAGS@ @SQLITE3_CFLAGS@
librecently2_la_LDFLAGS = @PACKAGE_LIBS@ @SQLITE3_LIBS@ -module -avoid-version
librecently2_la_SOURCES = recently.c

BUILD_SOURCES = dock.ui
EXTRA_DIST = dock.glade
CLEAN_SOURCES = dock.ui

dock.ui: dock.glade
	$(GTK_BUILDER_CONVERT) dock.glade dock.ui
