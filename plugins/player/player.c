/*
 * plugins/player/plugin.c
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
#include "eina-player.h"

G_MODULE_EXPORT gboolean
player_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	GtkVBox *content = gel_ui_application_get_window_content_area(application);

	GtkWidget *player = eina_player_new();
	gtk_box_pack_start ((GtkBox *) content, player, TRUE, TRUE, 0);
	gtk_widget_show_all(player);

	return TRUE;
}

G_MODULE_EXPORT gboolean
player_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

EINA_PLUGIN_INFO_SPEC(player,
	NULL,
	NULL,
	NULL,
	NULL,
	N_("Player plugin"),
	NULL,
	NULL
	);
