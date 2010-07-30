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
#include "plugins/playlist/playlist.h"
#include "plugins/dock/dock.h"

G_MODULE_EXPORT gboolean
playlist_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *playlist = eina_playlist_new();
	eina_playlist_set_lomo_player(playlist, gel_plugin_engine_get_interface(engine, "lomo"));
	gtk_widget_show((GtkWidget *) playlist);

	eina_dock_add_widget(gel_plugin_engine_get_interface(engine, "dock"),
		N_("Playlist"), gtk_label_new(N_("Playlist")), (GtkWidget *) playlist);

	return TRUE;
}

G_MODULE_EXPORT gboolean
playlist_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

