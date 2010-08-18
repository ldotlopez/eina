include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/art
plugin_LTLIBRARIES = libart.la
libart_la_CFLAGS  = @EINA_CFLAGS@
libart_la_LDFLAGS = @EINA_LIBS@
libart_la_SOURCES = \
	art.h \
	art.c

plugin_DATA = art.ini

