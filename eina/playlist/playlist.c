/*
 * plugins/playlist/playlist.c
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

#include "eina/eina-plugin2.h"
#include "eina/playlist/playlist.h"
#include "eina/dock/dock.h"
#include "eina/fs.h"

#define EINA_PLAYLIST_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.playlist"                     
#define EINA_PLAYLIST_STREAM_MARKUP_KEY "stream-markup"

static gboolean
action_activated_cb(EinaPlaylist *playlist, GtkAction *action, GelPluginEngine *engine);

G_MODULE_EXPORT gboolean
playlist_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	GSettings        *settings    = gel_ui_application_get_settings(application, EINA_PLAYLIST_PREFERENCES_DOMAIN);

	EinaPlaylist *playlist = eina_playlist_new();
	g_object_set(playlist,
		"lomo-player",    gel_plugin_engine_get_interface(engine, "lomo"),
		"stream-markup",  g_settings_get_string(settings, EINA_PLAYLIST_STREAM_MARKUP_KEY),
		NULL);
	g_signal_connect(playlist, "action-activated", (GCallback) action_activated_cb, engine);
	gtk_widget_show((GtkWidget *) playlist);

	eina_dock_add_widget(gel_plugin_engine_get_interface(engine, "dock"),
		N_("Playlist"), gtk_label_new(N_("Playlist")), (GtkWidget *) playlist);

	gel_plugin_set_data(plugin, playlist);

	return TRUE;
}

G_MODULE_EXPORT gboolean
playlist_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *playlist = (EinaPlaylist *) gel_plugin_get_data(plugin);
	g_return_val_if_fail(EINA_IS_PLAYLIST(playlist), FALSE);

	eina_dock_remove_widget(gel_plugin_engine_get_interface(engine, "dock"), N_("Playlist"));

	return TRUE;
}

static gboolean
action_activated_cb(EinaPlaylist *playlist, GtkAction *action, GelPluginEngine *engine)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal("add-action", name))
		eina_fs_load_from_default_file_chooser(engine);
	else
		return FALSE;

	return TRUE;
}
