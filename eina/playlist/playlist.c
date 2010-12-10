/*
 * eina/playlist/playlist.c
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

#include "playlist.h"

#include <eina/eina-plugin.h>
#include <eina/dock/dock.h>
#include <eina/ext/eina-fs.h>

#define EINA_PLAYLIST_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.playlist"                     
#define EINA_PLAYLIST_STREAM_MARKUP_KEY "stream-markup"

static gboolean
action_activated_cb(EinaPlaylist *playlist, GtkAction *action, EinaApplication *app);

G_MODULE_EXPORT gboolean
playlist_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	GSettings        *settings    = eina_application_get_settings(app, EINA_PLAYLIST_PREFERENCES_DOMAIN);

	EinaPlaylist *playlist = eina_playlist_new();
	g_object_set(playlist,
		"lomo-player",    eina_application_get_interface(app, "lomo"),
		"stream-markup",  g_settings_get_string(settings, EINA_PLAYLIST_STREAM_MARKUP_KEY),
		NULL);
	g_signal_connect(playlist, "action-activated", (GCallback) action_activated_cb, app);

	eina_dock_add_widget(eina_application_get_interface(app, "dock"),
		N_("Playlist"),
		gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU),
		(GtkWidget *) g_object_ref(playlist));

	gel_plugin_set_data(plugin, playlist);

	return TRUE;
}

G_MODULE_EXPORT gboolean
playlist_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *playlist = (EinaPlaylist *) gel_plugin_steal_data(plugin);
	g_return_val_if_fail(EINA_IS_PLAYLIST(playlist), FALSE);

	eina_dock_remove_widget(eina_application_get_interface(app, "dock"), (GtkWidget *) playlist);
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
