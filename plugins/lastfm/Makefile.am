lastfmdir = $(pkglibdir)/lastfm

lastfm_LTLIBRARIES = liblastfm.la
lastfm_DATA = lastfm.png lastfm.ui logo.gif

AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)
#if USE_LASTFM_WEBVIEW
#AM_CFLAGS += -DHAVE_WEBKIT=1 @WEBKITGTK_CFLAGS@
#else
#AM_CFLAGS += -DHAVE_WEBKIT=0
#endif

liblastfm_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
#if USE_LASTFM_WEBVIEW
#liblastfm_la_LDFLAGS += @WEBKITGTK_LIBS@
#endif

liblastfm_la_SOURCES = lastfm.c lastfm.h submit.c submit.h artwork.c artwork.h

#if USE_LASTFM_WEBVIEW
#liblastfm_la_SOURCES += webview.c webview.h
#endif

lastfmsubmitddir = $(lastfmdir)/lastfmsubmid
lastfmsubmitd_DATA = \
	lastfmsubmitd/lastfmsubmitd
lastfmsubmitdlibdir = $(lastfmsubmitddir)/lastfm
lastfmsubmitdlib_DATA = \
	lastfmsubmitd/lastfm/__init__.py   \
	lastfmsubmitd/lastfm/marshaller.py \
	lastfmsubmitd/lastfm/config.py     \
	lastfmsubmitd/lastfm/client.py

EXTRA_DIST = lastfm.png lastfm.ui logo.gif \
	$(lastfmsubmitd_DATA) \
	$(lastfmsubmitdlib_DATA)

