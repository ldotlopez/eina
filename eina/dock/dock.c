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

#include "dock.h"
#include <eina/eina-plugin2.h>
#include <eina/application/application.h>
#include <eina/player/player.h>

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

	GSettings *settings = gel_ui_application_get_settings(eina_plugin_get_application(plugin), EINA_DOCK_PREFERENCES_DOMAIN);

	data->dock = (GtkWidget *) g_object_ref(eina_dock_new());
	eina_dock_set_page_order((EinaDock *) data->dock, (gchar **) g_settings_get_strv(settings, EINA_DOCK_ORDER_KEY));
	g_settings_bind(settings, EINA_DOCK_ORDER_KEY, data->dock, "page-order", G_SETTINGS_BIND_DEFAULT);

	gtk_box_pack_start(GTK_BOX(gel_ui_application_get_window_content_area(application)), data->dock, TRUE, TRUE, 0);
	gtk_widget_show(data->dock);

	GtkWindow *win = GTK_WINDOW(gel_ui_application_get_window(application));
	data->resizable = gtk_window_get_resizable(win);
	g_object_bind_property(data->dock, "expanded", win, "resizable", G_BINDING_BIDIRECTIONAL|G_BINDING_BIDIRECTIONAL);

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
	g_object_unref(data->dock);
	g_free(data);

	return TRUE;
}

gboolean
eina_plugin_add_dock_widget(EinaPlugin *plugin, gchar *id, GtkWidget *label, GtkWidget *widget)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);

	return eina_dock_add_widget(dock, id, label, widget);
}

gboolean
eina_plugin_switch_dock_widget(EinaPlugin *plugin, gchar *id)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);

	return eina_dock_switch_widget(dock, id);
}

gboolean
eina_plugin_remove_dock_widget(EinaPlugin *plugin, GtkWidget *widget)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

	return eina_dock_remove_widget(dock, widget);
}

gboolean
eina_plugin_remove_dock_widget_by_id(EinaPlugin *plugin, gchar *id)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);
	g_return_val_if_fail(id, FALSE);

	return eina_dock_remove_widget_by_id(dock, id);
}


