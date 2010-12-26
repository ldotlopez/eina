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

	gint w, h;
	gboolean pre_resizable;
} DockData;

static gboolean
dock_save_size_in_idle(DockData *self)
{
	GtkWidget *window = gtk_widget_get_toplevel(self->dock);

	GtkAllocation alloc;
	gtk_widget_get_allocation(window, &alloc);
	self->w = alloc.width;
	self->h = alloc.height;

	return FALSE;
}

static gboolean
dock_configure_event_cb(GdkWindow *win, GdkEventConfigure *ev, DockData *self)
{
	g_idle_add((GSourceFunc) dock_save_size_in_idle, self);
	return FALSE;
}

static gboolean
dock_restore_size_in_idle(DockData *self)
{
	GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(self->dock));
	if (window)
		gtk_window_resize(window, self->w, self->h);
	return FALSE;
}

static void
dock_widget_add(EinaDock *dock, const gchar *id, DockData *self)
{
	if (eina_dock_get_n_widgets(dock) == 2)
	{
		// Restore expanded property and schedule the resize
		gboolean expanded = g_settings_get_boolean(self->settings, EINA_DOCK_EXPANDED_KEY);
		gtk_window_set_resizable(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dock))), expanded);
		eina_dock_set_expanded(dock, g_settings_get_boolean(self->settings, EINA_DOCK_EXPANDED_KEY));
		if (expanded)
			g_idle_add((GSourceFunc) dock_restore_size_in_idle, self);
	}
}

static void
dock_notify_cb(GtkWidget *dock, GParamSpec *pspec, DockData *self)
{
	if (g_str_equal(pspec->name, "resizable"))
	{
		gboolean v = eina_dock_get_resizable(EINA_DOCK(dock));
		GtkWidget *window = gtk_widget_get_toplevel(dock);
		if (v)
		{
			g_idle_add((GSourceFunc) dock_restore_size_in_idle, self);
 			g_signal_connect(gtk_widget_get_toplevel(self->dock), "configure-event", (GCallback) dock_configure_event_cb, self);
		}
		else
		{
			gtk_window_resize(GTK_WINDOW(window), 1, 1);
			g_signal_handlers_disconnect_by_func(window, dock_configure_event_cb, self);
		}
	}

	else
		g_warning(_("Unhandled notify::%s"), pspec->name);
}

G_MODULE_EXPORT gboolean
dock_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(app), FALSE);

	DockData *data = g_new0(DockData, 1);
	gel_plugin_set_data(plugin, data);

	data->settings = g_settings_new(EINA_DOCK_PREFERENCES_DOMAIN);

	// Setup dock
	data->dock = (GtkWidget *) g_object_ref(eina_dock_new());
	eina_dock_set_page_order((EinaDock *) data->dock, (gchar **) g_settings_get_strv(data->settings, EINA_DOCK_ORDER_KEY));
	g_settings_bind(data->settings, EINA_DOCK_ORDER_KEY, data->dock, "page-order", G_SETTINGS_BIND_DEFAULT);

	// Insert into applicatin
	GtkWindow *window = GTK_WINDOW(eina_application_get_window(app));
	gtk_container_add(GTK_CONTAINER(window), data->dock);
	data->pre_resizable = gtk_window_get_resizable(window);
	gtk_window_set_resizable(window, FALSE);
	gtk_widget_show(data->dock);

	// Load previous size
	data->w = g_settings_get_int(data->settings, EINA_DOCK_WINDOW_W_KEY);
	data->h = g_settings_get_int(data->settings, EINA_DOCK_WINDOW_H_KEY);

	g_object_bind_property(data->dock, "resizable", window, "resizable", 0);

	g_signal_connect(data->dock, "widget-add",        (GCallback) dock_widget_add, data);
	g_signal_connect(data->dock, "notify::resizable", (GCallback) dock_notify_cb,  data);

	eina_application_set_interface(app, "dock", data->dock);

	return TRUE;
}

G_MODULE_EXPORT gboolean
dock_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	DockData *data = gel_plugin_get_data(plugin);
	g_return_val_if_fail(EINA_IS_DOCK(data->dock), FALSE);

	GtkContainer *container = (GtkContainer *) gtk_widget_get_parent(data->dock);
	g_return_val_if_fail(container && GTK_IS_CONTAINER(container), FALSE);

	GtkWindow *window = (GtkWindow *) gtk_widget_get_toplevel(data->dock);
	g_return_val_if_fail(window && GTK_IS_WINDOW(window), FALSE);

	eina_application_set_interface(app, "dock", NULL);
	gel_plugin_set_data(plugin, NULL);

	gtk_window_set_resizable(window, data->pre_resizable);

	gtk_container_remove(container, data->dock);

	g_object_unref(data->settings);
	g_object_unref(data->dock);
	g_free(data);

	return TRUE;
}

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


