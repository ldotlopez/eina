/*
 * plugins/lastfm/lastfm.c
 *
 * Copyright (C) 2004-2010 Eina
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

#include <config.h>
#include "lastfm.h"
#include "submit.h"
#include "artwork.h"

#define ENABLE_WEBVIEW 0
#define SETTINGS_PATH "/lastfm"

struct _LastFMPriv {
	GtkBuilder *prefs_ui;
	GtkWidget  *prefs_widget;
};

GQuark
lastfm_quark(void)
{
	GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("eina-lastfm");
	return ret;
}

// --
// Main plugin
// --
gboolean
lastfm_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFM *self = g_new0(LastFM, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "lastfm", EINA_OBJ_NONE, error))
		return FALSE;
	self->priv = g_new0(LastFMPriv, 1);

	gchar *prefs_path = NULL;
	gchar *prefs_xml  = NULL;
	if ((prefs_path = gel_plugin_get_resource(plugin, GEL_RESOURCE_UI, "lastfm.ui")) &&
	     g_file_get_contents(prefs_path, &prefs_xml, NULL, NULL))
	{
		EinaPreferences *preferences = gel_app_get_preferences(app);
		gchar *objects[] = { "/lastfm/submit", "/lastfm/username", "/lastfm/password"};
		eina_preferences_add_tab_full(preferences, "lastfm", prefs_xml, "main-widget", objects, G_N_ELEMENTS(objects), 
			NULL, (GtkLabel*) gtk_label_new("Last FM"));
	}

	plugin->data = self;

	if (!lastfm_artwork_init(app, plugin, error))
		goto lastfm_init_fail;

	if (!lastfm_submit_init(app, plugin, error))
		goto lastfm_init_fail;

#if ENABLE_WEBVIEW
	GError *non_fatal_err = NULL;
	if (!lastfm_webview_init(app, plugin, &non_fatal_err))
	{
		gel_warn("Cannot init LastFM webview: %s", non_fatal_err->message);
		g_error_free(non_fatal_err);
	}
#endif

	const gchar *username = eina_conf_get_str(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH"/username", NULL);
	const gchar *password = eina_conf_get_str(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH"/password", NULL);
	gboolean account_ok = FALSE;
	if ((username != NULL) && (password != NULL))
		account_ok = lastfm_submit_set_account_info(self->submit, (gchar*) username, (gchar*) password);
	
	if (account_ok && eina_conf_get_bool(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH"/submit", FALSE))
		lastfm_submit_set_submit(self->submit, TRUE);
	else
		lastfm_submit_set_submit(self->submit, FALSE);

	return TRUE;

lastfm_init_fail:
	if (self->artwork)
		lastfm_artwork_fini(app, plugin, NULL);
	if (self->submit)
		lastfm_submit_fini(app, plugin, NULL);
	g_free(self);
	return FALSE;
}

gboolean
lastfm_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFM *self = EINA_PLUGIN_DATA(plugin);

	if (self->submit && !lastfm_submit_fini(app, plugin, error))
		return FALSE;

#if ENABLE_WEBVIEW
	if (self->webview && !lastfm_webview_fini(app, plugin, error))
		return FALSE;
#endif

	if (!lastfm_artwork_fini(app, plugin, error))
		return FALSE;

	g_free(plugin->data);
	self = plugin->data = NULL;

	return TRUE;
}


// --
// Export plugin
// --
EINA_PLUGIN_SPEC(lastfm,
	NULL, "lomo,settings",
	NULL, NULL,

	N_("Lastfm integration"),
	N_("Lastfm integration:\n"
	"· Query Last.fm for covers"),
	"lastfm.png",
	
	lastfm_init, lastfm_fini
);
