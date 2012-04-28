/*
 * eina/lastfm/lastfm-priv.h
 *
 * Copyright (C) 2004-2011 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EINA_LASTFM_PRIV_H
#define _EINA_LASTFM_PRIV_H

#include <glib.h>
#include <lomo/lomo-player.h>
#include <eina/preferences/eina-preferences-plugin.h>

#define LASTFM_PREFERENCES_DOMAIN EINA_APP_DOMAIN".preferences.lastfm"
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
} EinaLastfmPluginPrivate;

G_END_DECLS

#endif
