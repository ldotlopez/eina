/*
 * eina/playlist/eina-playlist-plugin.c
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

#include "eina-playlist-plugin.h"
#include <eina/ext/eina-fs.h>
#include <eina/ext/eina-extension.h>
#include <eina/dock/eina-dock-plugin.h>

#define EINA_TYPE_PLAYLIST_PLUGIN         (eina_playlist_plugin_get_type ())
#define EINA_PLAYLIST_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_PLAYLIST_PLUGIN, EinaPlaylistPlugin))
#define EINA_PLAYLIST_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_PLAYLIST_PLUGIN, EinaPlaylistPlugin))
#define EINA_IS_PLAYLIST_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_PLAYLIST_PLUGIN))
#define EINA_IS_PLAYLIST_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_PLAYLIST_PLUGIN))
#define EINA_PLAYLIST_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_PLAYLIST_PLUGIN, EinaPlaylistPluginClass))

typedef struct {
	EinaPlaylist *playlist_widget;
	EinaDockTab  *dock_tab;
} EinaPlaylistPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_PLAYLIST_PLUGIN, EinaPlaylistPlugin, eina_playlist_plugin)

#define EINA_PLAYLIST_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.playlist"                     
#define EINA_PLAYLIST_STREAM_MARKUP_KEY "stream-markup"

static gboolean
action_activated_cb(EinaPlaylist *playlist, GtkAction *action, EinaApplication *app);

static gboolean
eina_playlist_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaPlaylistPlugin *plugin = EINA_PLAYLIST_PLUGIN(activatable);
	g_return_val_if_fail(EINA_IS_PLAYLIST_PLUGIN(plugin), FALSE);

	EinaPlaylistPluginPrivate *priv = plugin->priv;

	GSettings *settings = eina_application_get_settings(app, EINA_PLAYLIST_PREFERENCES_DOMAIN);

	priv->playlist_widget = eina_playlist_new(eina_application_get_interface(app, "lomo"));
	eina_playlist_set_stream_markup(priv->playlist_widget, g_settings_get_string(settings, EINA_PLAYLIST_STREAM_MARKUP_KEY));
	g_signal_connect(priv->playlist_widget, "action-activated", (GCallback) action_activated_cb, app);

	priv->dock_tab = eina_application_add_dock_widget(app,
		N_("Playlist"),
		(GtkWidget *) g_object_ref(priv->playlist_widget),
		gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU),
		EINA_DOCK_DEFAULT);

	return TRUE;
}

static gboolean
eina_playlist_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaPlaylistPlugin *plugin = EINA_PLAYLIST_PLUGIN(activatable);
	g_return_val_if_fail(EINA_IS_PLAYLIST_PLUGIN(plugin), FALSE);

	EinaPlaylistPluginPrivate *priv = plugin->priv;

	g_warn_if_fail(EINA_IS_DOCK_TAB(priv->dock_tab));
	if (EINA_IS_DOCK_TAB(priv->dock_tab))
	{
		eina_application_remove_dock_widget(app, priv->dock_tab);
		priv->dock_tab = NULL;
	}

	g_warn_if_fail(EINA_IS_PLAYLIST(priv->playlist_widget));
	if (EINA_IS_PLAYLIST(priv->playlist_widget))
	{
		g_object_unref(priv->playlist_widget);
		priv->playlist_widget = NULL;
	}

	return TRUE;
}

static gboolean
action_activated_cb(EinaPlaylist *playlist, GtkAction *action, EinaApplication *app)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal("add-action", name))
		eina_fs_load_from_default_file_chooser(app);
	else
		return FALSE;

	return TRUE;
}
