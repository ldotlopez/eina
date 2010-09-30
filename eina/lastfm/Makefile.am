include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/lastfm
plugin_LTLIBRARIES = liblastfm.la
liblastfm_la_CFLAGS  = @EINA_CFLAGS@
liblastfm_la_LDFLAGS = @EINA_LIBS@
liblastfm_la_SOURCES = \
	lastfm.c

plugin_DATA = lastfm.ini

EXTRA_DIST += lastfm.ini
