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
	gboolean   resizable;
	GSettings *settings;
	GBinding  *window_bind;

	gint w, h;
} DockData;

static gboolean
dock_save_current_size(DockData *self)
{
	GtkAllocation alloc;
	gtk_widget_get_allocation(self->dock, &alloc);
	g_warning("Current size: %dx%d", alloc.width, alloc.height);
	self->w = alloc.width;
	self->h = alloc.height;

	return FALSE;
}

static gboolean
dock_configure_event_cb(GdkWindow *win, GdkEventConfigure *ev, DockData *self)
{
	g_idle_add((GSourceFunc) dock_save_current_size, self);
	return FALSE;
}

static void
dock_widget_add(EinaDock *dock, const gchar *id, DockData *self)
{
	g_warning("Restore expandable after add the first widget (id: %s)", id);
	g_signal_handlers_disconnect_by_func(dock, dock_widget_add, self);
}

static gboolean
dock_enable_configure_event_watcher(DockData *self)
{
	return FALSE;
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
			g_warning("Restoring previous size %dx%d", self->w, self->h);

			gint border = gtk_container_get_border_width(GTK_CONTAINER(self->dock));
			gtk_widget_set_size_request(self->dock, self->w + border*2, self->h + border*2);

			gtk_widget_queue_resize(self->dock);
 			g_signal_connect(gtk_widget_get_toplevel(self->dock), "configure-event", (GCallback) dock_configure_event_cb, self);
			if (0) g_idle_add((GSourceFunc) dock_enable_configure_event_watcher, self);
		}
		else
		{
			gtk_widget_set_size_request(self->dock, 1, 1);
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
	gtk_widget_show(data->dock);

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

	g_object_unref(data->window_bind);
	gtk_window_set_resizable(window, data->resizable);

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

#if RESIZE_WINDOW
static void
dock_activate_cb(GtkExpander *expander, DockData *self)
{
	gboolean resizable = !gtk_expander_get_expanded(expander);

	if (!resizable)
	{
		gint w, h;
		GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel((GtkWidget *) expander));

		gtk_window_get_size(window, &w, &h);
		g_settings_set_int(self->settings, EINA_DOCK_WINDOW_W_KEY, w);
		g_settings_set_int(self->settings, EINA_DOCK_WINDOW_H_KEY, h);
		g_signal_handlers_disconnect_by_func(self->dock, dock_draw_cb, self);

		GdkWindowState state = gdk_window_get_state(gtk_widget_get_window((GtkWidget *) window));
		if (state & (GDK_WINDOW_STATE_MAXIMIZED|GDK_WINDOW_STATE_FULLSCREEN))
		{
			gtk_window_unmaximize(window);
			gtk_window_unfullscreen(window);
		}
	}
	else
		g_signal_connect(self->dock, "draw", (GCallback) dock_draw_cb, self);
}

static gboolean
dock_draw_cb(GtkWidget *widget, cairo_t *cr, DockData *self)
{
	gtk_window_resize(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
		g_settings_get_int(self->settings, EINA_DOCK_WINDOW_W_KEY),
		g_settings_get_int(self->settings, EINA_DOCK_WINDOW_H_KEY));
	g_signal_handlers_disconnect_by_func(self->dock, dock_draw_cb, self);

	return FALSE;
}

#endif
