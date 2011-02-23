/*
 * eina/dock/dock.c
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
#include <eina/player/player.h>

typedef struct {
	GtkWidget *dock;
	GSettings *settings;

	gboolean prev_state;
	gboolean delayed_open;
} DockPlugin;

EinaDockTab*
eina_plugin_add_dock_widget(EinaPlugin *plugin, const gchar *id, GtkWidget *widget, GtkWidget *label, EinaDockFlags flags);
gboolean
eina_plugin_switch_dock_widget(EinaPlugin *plugin, EinaDockTab *tab);
gboolean
eina_plugin_remove_dock_widget(EinaPlugin *plugin, EinaDockTab *tab);

static void
dock_widget_add_cb(EinaDock *dock, const gchar *name, DockPlugin *plugin);
static void
dock_notify_cb(EinaDock *dock, GParamSpec *pspec, DockPlugin *plugin);

G_MODULE_EXPORT gboolean
dock_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(app), FALSE);

	DockPlugin *data = g_new0(DockPlugin, 1);
	gel_plugin_set_data(plugin, data);

	data->settings = g_settings_new(EINA_DOCK_PREFERENCES_DOMAIN);

	// Setup dock
	data->dock = (GtkWidget *) g_object_ref(eina_dock_new());
	data->prev_state = eina_dock_get_resizable(EINA_DOCK(data->dock));

	// Insert into applicatin
	GtkWindow *window = GTK_WINDOW(eina_application_get_window(app));
	gtk_container_add(GTK_CONTAINER(window), data->dock);

	eina_dock_set_page_order((EinaDock *) data->dock, (gchar **) g_settings_get_strv(data->settings, EINA_DOCK_ORDER_KEY));
	g_settings_bind(data->settings, EINA_DOCK_ORDER_KEY, data->dock, "page-order", G_SETTINGS_BIND_DEFAULT);

	g_object_bind_property(data->dock, "resizable", window, "resizable", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
	g_signal_connect(data->dock, "notify::resizable", (GCallback) dock_notify_cb,  data);

	data->delayed_open = g_settings_get_boolean(data->settings, EINA_DOCK_EXPANDED_KEY);
	g_settings_bind(data->settings, EINA_DOCK_EXPANDED_KEY, data->dock, "resizable", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(data->dock, "widget-add", (GCallback) dock_widget_add_cb, data);

	// Run
	gtk_widget_show(data->dock);
	eina_application_set_interface(app, "dock", data->dock);

	return TRUE;
}

G_MODULE_EXPORT gboolean
dock_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	DockPlugin *data = gel_plugin_get_data(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(data->dock), FALSE);

	GtkContainer *container = (GtkContainer *) gtk_widget_get_parent(data->dock);
	g_return_val_if_fail(container && GTK_IS_CONTAINER(container), FALSE);

	GtkWindow *window = (GtkWindow *) gtk_widget_get_toplevel(data->dock);
	g_return_val_if_fail(window && GTK_IS_WINDOW(window), FALSE);

	eina_application_set_interface(app, "dock", NULL);
	gel_plugin_set_data(plugin, NULL);

	gtk_container_remove(container, data->dock);

	g_object_unref(data->settings);
	g_object_unref(data->dock);
	g_free(data);

	return TRUE;
}

/*
 * Internal API
 */
static void 
save_size(DockPlugin *plugin)
{
	g_return_if_fail(G_IS_SETTINGS(plugin->settings));

	GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(plugin->dock));
	g_return_if_fail(window != NULL);

	gint w, h;
	gtk_window_get_size(window, &w, &h);

	g_settings_set_int(plugin->settings, EINA_DOCK_WINDOW_W_KEY, w);
	g_settings_set_int(plugin->settings, EINA_DOCK_WINDOW_H_KEY, h);
}

static void 
restore_size(DockPlugin *plugin)
{
	g_return_if_fail(G_IS_SETTINGS(plugin->settings));

	GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(plugin->dock));
	g_return_if_fail(GTK_IS_WINDOW(window));

	gtk_window_resize(window,
		g_settings_get_int(plugin->settings, EINA_DOCK_WINDOW_W_KEY),
		g_settings_get_int(plugin->settings, EINA_DOCK_WINDOW_H_KEY));
}

/*
 * Plugin API
 */
EinaDockTab*
eina_plugin_add_dock_widget(EinaPlugin *plugin, const gchar *id, GtkWidget *widget, GtkWidget *label, EinaDockFlags flags)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);

	return eina_dock_add_widget(dock, id, widget, label, flags);
}

gboolean
eina_plugin_switch_dock_widget(EinaPlugin *plugin, EinaDockTab *tab)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);
	g_return_val_if_fail(EINA_IS_DOCK_TAB(tab), FALSE);

	return eina_dock_switch_widget(dock, tab);
}

gboolean
eina_plugin_remove_dock_widget(EinaPlugin *plugin, EinaDockTab *tab)
{
	EinaDock *dock = eina_plugin_get_dock(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);
	g_return_val_if_fail(EINA_IS_DOCK_TAB(tab), FALSE);

	return eina_dock_remove_widget(dock, tab);
}

/*
 * Event callbacks
 */
static void
dock_widget_add_cb(EinaDock *dock, const gchar *name, DockPlugin *plugin)
{
	if ((eina_dock_get_n_widgets(dock) == 2) && plugin->delayed_open)
	{
		eina_dock_set_resizable(dock, TRUE);
		plugin->delayed_open = FALSE;
	}
}

static void
dock_notify_cb(EinaDock *dock, GParamSpec *pspec, DockPlugin *plugin)
{
	if (g_str_equal(pspec->name, "resizable"))
	{
		gboolean resizable = eina_dock_get_resizable(dock);
		if (resizable == plugin->prev_state)
			return;

		if (resizable == TRUE)
			restore_size(plugin);
		else
			save_size(plugin);

		plugin->prev_state = resizable;
	}
}

