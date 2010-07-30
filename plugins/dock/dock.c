/*
 * plugins/dock/dock.c
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
#include "eina-dock.h"
#include "plugins/player/eina-player.h"

G_MODULE_EXPORT gboolean
dock_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GtkWidget *dock = (GtkWidget *) eina_dock_new();

	EinaPlayer *player = gel_plugin_engine_get_interface(engine, "player");

	gtk_box_pack_start(GTK_BOX(player), dock, TRUE, TRUE, 0);
	gtk_widget_show(dock);

	gel_plugin_engine_set_interface(engine, "dock", dock);

	return TRUE;
}

G_MODULE_EXPORT gboolean
dock_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

