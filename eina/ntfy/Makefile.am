include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/ntfy
plugin_LTLIBRARIES = libntfy.la
libntfy_la_CFLAGS  = @EINA_CFLAGS@ @NOTIFY_CFLAGS@
libntfy_la_LDFLAGS = @EINA_LIBS@ @NOTIFY_LIBS@ -module -avoid-version
libntfy_la_SOURCES =   \
	eina-ntfy-plugin.h \
	eina-ntfy-plugin.c 

plugin_DATA = \
	ntfy.png  \
	ntfy.plugin

EXTRA_DIST += \
	$(plugin_DATA) \
	ntfy.svg

-include $(top_srcdir)/git.mk
