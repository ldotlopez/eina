include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/coverplus
plugin_LTLIBRARIES = libcoverplus.la
libcoverplus_la_CFLAGS  = @EINA_CFLAGS@
libcoverplus_la_LDFLAGS = @EINA_LIBS@ -module -avoid-version
libcoverplus_la_SOURCES = \
	iface.c          \
	banshee.h        \
	banshee.c        \
	infolder.h       \
	infolder-sync.c

