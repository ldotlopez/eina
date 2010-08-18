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

typedef struct {
	GtkWidget *dock;
	gboolean   resizable;
} DockData;

G_MODULE_EXPORT gboolean
dock_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(application), FALSE);

	DockData *data = g_new0(DockData, 1);
	gel_plugin_set_data(plugin, data);

	data->dock = (GtkWidget *) eina_dock_new();
	gtk_box_pack_start(GTK_BOX(gel_ui_application_get_window_content_area(application)), data->dock, TRUE, TRUE, 0);
	gtk_widget_show(data->dock);

	GtkWindow *win = GTK_WINDOW(gel_ui_application_get_window(application));
	data->resizable = gtk_window_get_resizable(win);
	gtk_window_set_resizable(win, TRUE);

	gel_plugin_engine_set_interface(engine, "dock", data->dock);

	return TRUE;
}

G_MODULE_EXPORT gboolean
dock_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	DockData *data = gel_plugin_get_data(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(data->dock), FALSE);

	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(application), FALSE);

	GtkWindow *win = gel_ui_application_get_window(application);
	g_return_val_if_fail(GTK_IS_WINDOW(win), FALSE);

	GtkContainer *content_area = (GtkContainer *) gel_ui_application_get_window_content_area(application);
	g_return_val_if_fail(GTK_IS_CONTAINER(content_area), FALSE);

	gel_plugin_engine_set_interface(engine, "dock", NULL);

	gtk_container_remove(content_area, data->dock);
	gtk_window_set_resizable(win, data->resizable);
	g_free(data);

	return TRUE;
}

