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
#define SETTINGS_PATH "/plugins/lastfm"

struct _LastFMPriv {
	GtkBuilder *prefs_ui;
	GtkWidget  *prefs_widget;
};

GtkWidget *
lastfm_prefs_new(LastFM *self);
static gboolean
focus_out_event_entry_cb(GtkWidget *w, GdkEventFocus *ev, LastFM *self);
static void
checkbutton_toggle_cb(GtkToggleButton *w, LastFM *self);

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

	GtkWidget *panel = lastfm_prefs_new(self);
	if (panel)
		eina_plugin_add_configuration_widget(plugin, NULL, (GtkLabel*) gtk_label_new("LastFM"), panel);

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
// Preferences dialog
// --
struct SignalDef {
	gchar *name;
	gchar *signal;
	GCallback callback;
};

GtkWidget *
lastfm_prefs_new(LastFM *self)
{
	if (self->priv->prefs_widget)
		return self->priv->prefs_widget;

	GError *error = NULL;

	gchar *uipath = gel_plugin_get_resource(eina_obj_get_plugin(self), GEL_RESOURCE_UI, "lastfm.ui");
	g_return_val_if_fail(uipath != NULL, NULL);

	self->priv->prefs_ui = gtk_builder_new();
	if (gtk_builder_add_from_file(self->priv->prefs_ui, uipath, &error) == 0)
	{
		gel_error(N_("Cannot load preferences UI: %s"), error->message);
		goto lastfm_prefs_new_fail;
	}

	if ((self->priv->prefs_widget = GTK_WIDGET(gtk_builder_get_object(self->priv->prefs_ui, "main-container"))) == NULL)
	{
		gel_error(N_("Object main-container not found in UI"));
		goto lastfm_prefs_new_fail;
	}

	// Set values
	GtkBuilder *ui   = self->priv->prefs_ui;
	EinaConf   *conf = EINA_OBJ_GET_SETTINGS(self);

	const gchar *username = eina_conf_get_str(conf, SETTINGS_PATH "/username", NULL);
	const gchar *password = eina_conf_get_str(conf, SETTINGS_PATH "/password", NULL);
	if (username)
		g_object_set(gtk_builder_get_object(ui, "username-entry"), "text", username, NULL);
	if (password)
		g_object_set(gtk_builder_get_object(ui, "password-entry"), "text", password, NULL);

	gboolean active = eina_conf_get_bool(conf, SETTINGS_PATH "/submit", TRUE);
	g_object_set(gtk_builder_get_object(ui, "submit-checkbutton"),
		"active", active,
		NULL);
	g_object_set(gtk_builder_get_object(ui, "username-entry"),
		"sensitive", active,
		NULL);
	g_object_set(gtk_builder_get_object(ui, "password-entry"),
		"sensitive", active,
		NULL);

	// Connect signals
	struct SignalDef signals[] = {
		{ "username-entry",     "focus-out-event", (GCallback) focus_out_event_entry_cb },
		{ "password-entry",     "focus-out-event", (GCallback) focus_out_event_entry_cb },
		{ "submit-checkbutton", "toggled",         (GCallback) checkbutton_toggle_cb    },
	};
	gint i;
	for (i = 0; i < G_N_ELEMENTS(signals); i++)
		g_signal_connect(gtk_builder_get_object(self->priv->prefs_ui, signals[i].name), signals[i].signal, signals[i].callback, self);

	gtk_widget_unparent(self->priv->prefs_widget);
	g_object_unref(gtk_builder_get_object(self->priv->prefs_ui, "main-window"));

	gtk_widget_show(self->priv->prefs_widget);
	return self->priv->prefs_widget;

lastfm_prefs_new_fail:
	gel_free_and_invalidate(uipath,    NULL, g_free);
	gel_free_and_invalidate(error,     NULL, g_error_free);
	gel_free_and_invalidate(self->priv->prefs_ui, NULL, g_object_unref);
	return NULL;
}

static gboolean
focus_out_event_entry_cb(GtkWidget *w, GdkEventFocus *ev, LastFM *self)
{
	GtkBuilder *ui = self->priv->prefs_ui;
	const gchar *username = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(ui, "username-entry")));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(ui, "password-entry")));

	lastfm_submit_set_account_info(self->submit, (gchar*) username, (gchar*) password);

	eina_conf_set_str(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH"/username", (gchar*) username);
	eina_conf_set_str(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH"/password", (gchar*) password);

	return FALSE;
}

static void
checkbutton_toggle_cb(GtkToggleButton *w, LastFM *self)
{
	gboolean active = gtk_toggle_button_get_active(w);

	lastfm_submit_set_submit(self->submit, active);

	eina_conf_set_bool(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH"/submit", active);

	g_object_set(gtk_builder_get_object(self->priv->prefs_ui, "username-entry"), "sensitive", active, NULL);
	g_object_set(gtk_builder_get_object(self->priv->prefs_ui, "password-entry"), "sensitive", active, NULL);
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
