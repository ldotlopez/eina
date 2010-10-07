#ifndef _EINA_LASTFM_PRIV_H
#define _EINA_LASTFM_PRIV_H

#include <glib.h>
#include <lomo/lomo-player.h>
#include <eina/preferences/preferences.h>

#define LASTFM_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.lastfm"
#define LASTFM_SUBMIT_ENABLED_KEY "submit-enabled"
#define LASTFM_USERNAME_KEY       "username"
#define LASTFM_PASSWORD_KEY       "password"

enum {
    EINA_LASTFM_ERROR_START_DAEMON = 1
};

typedef struct {
	EinaPreferencesTab *prefs_tab;

	gchar      *daemonpath;
	GPid        daemonpid;

	GIOChannel *io_out, *io_err;
	guint       out_id, err_id;

	LomoPlayer *lomo;

	GSettings *settings;

	guint config_update_id;
} EinaLastFM;

G_END_DECLS

#endif
