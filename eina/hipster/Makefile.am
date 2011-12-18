include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/hipster
plugin_LTLIBRARIES = libhipster.la

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/hipster
include_HEADERS =

libhipster_la_CFLAGS  = @EINA_CFLAGS@ @CLASTFM_CFLAGS@
libhipster_la_LDFLAGS = @EINA_LIBS@ @CLASTFM_LIBS@ -module -avoid-version
libhipster_la_SOURCES = \
	$(include_HEADERS) \
	eina-hipster-plugin.c

plugin_DATA = hipster.plugin
EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
