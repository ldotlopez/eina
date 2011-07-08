include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/dbus
plugin_LTLIBRARIES = libdbus.la

include_HEADERS = 

libdbus_la_CFLAGS  = @EINA_CFLAGS@
libdbus_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libdbus_la_SOURCES = \
	$(include_HEADERS) \
	eina-dbus-plugin.c

plugin_DATA = dbus.plugin
EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
