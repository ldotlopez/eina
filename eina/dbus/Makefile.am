include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/dbus
plugin_LTLIBRARIES = libdbus.la

#includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/dbus
include_HEADERS = 
#	dbus.h

libdbus_la_CFLAGS  = @EINA_CFLAGS@
libdbus_la_LDFLAGS = @EINA_LIBS@
libdbus_la_SOURCES = \
	$(include_HEADERS) \
	lomo-player-instropection.xml.h \
	dbus.c

plugin_DATA = dbus.ini
EXTRA_DIST += dbus.ini
