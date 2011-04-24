/*
 * plugins/lastfm/lastfm.c
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

#include <config.h>
#include "lastfm.h"
#include "submit.h"
#include "artwork.h"

#define ENABLE_ARTWORK 0
#define ENABLE_WEBVIEW 0

struct _LastFMPriv {
	EinaPreferencesTab *prefs_tab;
};

static void
settings_changed_cb(GSettings *settings, gchar *key, LastFM *self);

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
lastfm_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFM *self = g_new0(LastFM, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "lastfm", EINA_OBJ_NONE, error))
		return FALSE;
	self->priv = g_new0(LastFMPriv, 1);

	gchar *prefs_path = NULL;
	gchar *prefs_ui_string = NULL;
	if ((prefs_path = gel_plugin_get_resource(plugin, GEL_RESOURCE_UI, "lastfm.ui")) &&
	     g_file_get_contents(prefs_path, &prefs_ui_string, NULL, NULL))
	{
		gchar *logo_path = gel_plugin_get_resource(plugin, GEL_RESOURCE_IMAGE, "logo.gif");

		self->priv->prefs_tab = eina_preferences_tab_new();
		g_object_set(self->priv->prefs_tab,
			"ui-string", prefs_ui_string,
			"label-text", "Last FM",
			"label-image", gtk_image_new_from_file(logo_path),
			NULL);
		g_free(prefs_ui_string);
		g_free(logo_path);

		eina_preferences_add_tab(gel_app_get_preferences(app), self->priv->prefs_tab);

		GSettings *settings = gel_app_get_settings(app, EINA_PLUGIN_LASTFM_PREFERENCES_DOMAIN);
		g_settings_bind(settings, EINA_PLUGIN_LASTFM_ENABLE_SUBMIT_KEY, eina_preferences_tab_get_widget(self->priv->prefs_tab, "submit-enabled"), "active", G_SETTINGS_BIND_DEFAULT);
		
		GtkWidget *u = eina_preferences_tab_get_widget(self->priv->prefs_tab, "username");
		GtkWidget *p = eina_preferences_tab_get_widget(self->priv->prefs_tab, "password");
		g_settings_bind(settings, EINA_PLUGIN_LASTFM_USERNAME_KEY, u, "text", G_SETTINGS_BIND_DEFAULT);
		g_settings_bind(settings, EINA_PLUGIN_LASTFM_PASSWORD_KEY, p, "text", G_SETTINGS_BIND_DEFAULT);

		gboolean enabled = g_settings_get_boolean(settings, EINA_PLUGIN_LASTFM_ENABLE_SUBMIT_KEY);
		g_object_set(u, "sensitive", enabled, NULL);
		g_object_set(p, "sensitive", enabled, NULL);

		g_signal_connect(settings, "changed", (GCallback) settings_changed_cb, self);
	}

	gel_plugin_set_data(plugin , self);

	if (!lastfm_submit_init(app, plugin, error))
		goto lastfm_init_fail;

#if ENABLE_ARTWORK
	if (!lastfm_artwork_init(app, plugin, error))
		goto lastfm_init_fail;
#endif

#if ENABLE_WEBVIEW
	GError *non_fatal_err = NULL;
	if (!lastfm_webview_init(app, plugin, &non_fatal_err))
	{
		gel_warn("Cannot init LastFM webview: %s", non_fatal_err->message);
		g_error_free(non_fatal_err);
	}
#endif

	return TRUE;

lastfm_init_fail:
#if ENABLE_ARTWORK
	if (self->artwork)
		lastfm_artwork_fini(app, plugin, NULL);
#endif
	if (self->submit)
		lastfm_submit_fini(app, plugin, NULL);
	g_free(self);
	return FALSE;
}

gboolean
lastfm_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFM *self = EINA_PLUGIN_DATA(plugin);

	if (self->submit && !lastfm_submit_fini(app, plugin, error))
		return FALSE;

#if ENABLE_WEBVIEW
	if (self->webview && !lastfm_webview_fini(app, plugin, error))
		return FALSE;
#endif

#if ENABLE_ARTWORK
	if (!lastfm_artwork_fini(app, plugin, error))
		return FALSE;
#endif

	eina_obj_fini((EinaObj *) self);
	gel_plugin_set_data(plugin, NULL);

	return TRUE;
}

static void
settings_changed_cb(GSettings *settings, gchar *key, LastFM *self)
{
	if (g_str_equal(key, EINA_PLUGIN_LASTFM_ENABLE_SUBMIT_KEY))
	{
		gboolean enabled = g_settings_get_boolean(settings, key);
		lastfm_submit_set_submit(self->submit, enabled);
		g_object_set(eina_preferences_tab_get_widget(self->priv->prefs_tab, "username"),
			"sensitive", enabled,
			NULL);
		g_object_set(eina_preferences_tab_get_widget(self->priv->prefs_tab, "password"),
			"sensitive", enabled,
			NULL);
		return;
	}

	if (g_str_equal(key, EINA_PLUGIN_LASTFM_USERNAME_KEY) ||
	    g_str_equal(key, EINA_PLUGIN_LASTFM_PASSWORD_KEY))
	{
		gchar *u = g_settings_get_string(settings, EINA_PLUGIN_LASTFM_USERNAME_KEY);
		gchar *p = g_settings_get_string(settings, EINA_PLUGIN_LASTFM_PASSWORD_KEY);
		lastfm_submit_set_account_info(self->submit, u, p);
		return;
	}
	g_warning(N_("Unhanded key '%s'"), key);
}

// --
// Export plugin
// --
EINA_PLUGIN_INFO_SPEC(lastfm,
	NULL, "lomo,settings",
	NULL, NULL,

	N_("Lastfm integration"),
	N_("Lastfm integration:\n"
	"· Query Last.fm for covers"),
	"lastfm.png"
);
