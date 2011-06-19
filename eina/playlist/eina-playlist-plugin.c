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
#include <eina/dock/eina-dock-plugin.h>

#define EINA_PLAYLIST_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.playlist"                     
#define EINA_PLAYLIST_STREAM_MARKUP_KEY "stream-markup"

static gboolean
action_activated_cb(EinaPlaylist *playlist, GtkAction *action, EinaApplication *app);

EINA_DEFINE_EXTENSION(EinaPlaylistPlugin, eina_playlist_plugin, EINA_TYPE_PLAYLIST_PLUGIN);

static gboolean
eina_playlist_plugin_activate(EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	GSettings *settings = eina_application_get_settings(app, EINA_PLAYLIST_PREFERENCES_DOMAIN);

	EinaPlaylist *playlist = eina_playlist_new(eina_application_get_interface(app, "lomo"));
	eina_playlist_set_stream_markup(playlist, g_settings_get_string(settings, EINA_PLAYLIST_STREAM_MARKUP_KEY));
	g_signal_connect(playlist, "action-activated", (GCallback) action_activated_cb, app);

	EinaDockTab *tab = eina_application_add_dock_widget(app,
		N_("Playlist"),
		(GtkWidget *) g_object_ref(playlist),
		gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU),
		EINA_DOCK_DEFAULT);
	g_object_set_data((GObject *) playlist, "x-playlist-dock-tab", tab);

	eina_activatable_set_data(plugin, playlist);

	return TRUE;
}

static gboolean
eina_playlist_plugin_deactivate(EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaPlaylist *playlist = (EinaPlaylist *) eina_activatable_steal_data(plugin);
	g_return_val_if_fail(EINA_IS_PLAYLIST(playlist), FALSE);

	EinaDockTab *tab = (EinaDockTab *) g_object_get_data((GObject *) playlist, "x-playlist-dock-tab");
	g_return_val_if_fail(EINA_IS_DOCK_TAB(tab), FALSE);
	
	eina_application_remove_dock_widget(app, tab);

	g_object_unref(playlist);

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
