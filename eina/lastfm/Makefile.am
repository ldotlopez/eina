include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/lastfm
plugin_LTLIBRARIES = liblastfm.la
liblastfm_la_CFLAGS  = @EINA_CFLAGS@
liblastfm_la_LDFLAGS = @EINA_LIBS@
liblastfm_la_SOURCES = \
	lastfm.c

plugin_DATA = lastfm.ini

EXTRA_DIST += lastfm.ini


lastfmsubmitddir = $(plugindir)/lastfmsubmitd
lastfmsubmitd_DATA = \
	lastfmsubmitd/lastfmsubmitd \
	lastfmsubmitd/COPYRIGHT \
	lastfmsubmitd/info.txt
lastfmsubmitdlibdir = $(lastfmsubmitddir)/lastfm
lastfmsubmitdlib_DATA = \
	lastfmsubmitd/lastfm/__init__.py   \
	lastfmsubmitd/lastfm/marshaller.py \
	lastfmsubmitd/lastfm/config.py     \
	lastfmsubmitd/lastfm/client.py
EXTRA_DIST += \
	$(lastfmsubmitd_DATA) \
	$(lastfmsubmitdlib_DATA)

install-data-hook:
	chmod +x $(DESTDIR)/$(plugindir)/lastfmsubmitd/lastfmsubmitd

