include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/osx
plugin_LTLIBRARIES = libosx.la

libosx_la_CFLAGS  = @EINA_CFLAGS@ @GTKMAC_CFLAGS@
libosx_la_LDFLAGS = @EINA_LIBS@ @GTKMAC_LIBS@ -module -avoid-version
libosx_la_SOURCES = \
	eina-osx-plugin.c

plugin_DATA = osx.plugin
EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
