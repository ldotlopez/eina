include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/recently
plugin_LTLIBRARIES = librecently.la
librecently_la_CFLAGS  = @EINA_CFLAGS@
librecently_la_LDFLAGS = @EINA_LIBS@
librecently_la_SOURCES = \
	recently.c

plugin_DATA = \
	dock.ui \
	recently.ini

EXTRA_DIST += $(plugin_DATA)

