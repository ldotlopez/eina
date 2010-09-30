#ifndef _EINA_LASTFM_PRIV_H
#define _EINA_LASTFM_PRIV_H

typedef struct {
	gboolean dummy;
#if 0
	// Subplugins
	struct _LastFMPrefs   *prefs;
	struct _LastFMSubmit  *submit;
	struct _LastFMArtwork *artwork;
#if HAVE_WEBKIT
	struct _LastFMWebView *webview;
#endif
#endif
} EinaLastFMData;

G_END_DECLS

#endif
