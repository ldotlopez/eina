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
} DockData;

#define RESIZE_WINDOW 0

#if RESIZE_WINDOW
static void
dock_activate_cb(GtkExpander *expander, DockData *self);
static gboolean
dock_draw_cb(GtkWidget *widget, cairo_t *cr, DockData *self);
#endif

static gboolean
dock_configure_event_cb(EinaDock *widget, GdkEventConfigure *ev, DockData *self);
static void
dock_notify_cb(EinaDock *widget, GParamSpec *pspec, DockData *self);

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
	// gtk_box_pack_start(GTK_BOX(eina_application_get_window_content_area(app)), data->dock, TRUE, TRUE, 0);
	GtkWindow *window = GTK_WINDOW(eina_application_get_window(app));
	gtk_container_add(GTK_CONTAINER(window), data->dock);
	gtk_widget_show(data->dock);

	// Setup dock widget 'expanded' prop and bind it to settings
	if (eina_dock_get_resizable((EinaDock *) data->dock))
	{
		GtkAllocation alloc;
		gtk_widget_get_allocation(data->dock, &alloc);
		g_settings_set_int(data->settings, EINA_DOCK_WINDOW_W_KEY, alloc.width);
		g_settings_set_int(data->settings, EINA_DOCK_WINDOW_H_KEY, alloc.height);
	}
	g_settings_bind(data->settings, EINA_DOCK_EXPANDED_KEY, data->dock, "resizable", 0);
	
	// Setup window for configure-event
	gtk_widget_realize(data->dock);
	gdk_window_set_events(gtk_widget_get_window(data->dock),
		GDK_STRUCTURE_MASK|gdk_window_get_events(gtk_widget_get_window(data->dock)));
	g_signal_connect(data->dock, "configure-event", (GCallback) dock_configure_event_cb, data);
	g_signal_connect(data->dock, "notify::resizable", (GCallback) dock_notify_cb, data);
	g_object_bind_property(data->dock, "resizable", window, "resizable", G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
/*	gtk_expander_set_expanded(GTK_EXPANDER(data->dock), g_settings_get_boolean(data->settings, EINA_DOCK_EXPANDED_KEY));
	g_settings_bind(data->settings, EINA_DOCK_EXPANDED_KEY, data->dock, "expanded", G_SETTINGS_BIND_DEFAULT);
*/
	// Setup window 'resizable' prop and bind it.
/*
	data->resizable = gtk_window_get_resizable(window);

	gtk_window_set_resizable(window, gtk_expander_get_expanded(GTK_EXPANDER(data->dock)));
	data->window_bind = g_object_bind_property(
		data->dock, "expanded",
		window, "resizable",
		G_BINDING_BIDIRECTIONAL);

*/
	#if RESIZE_WINDOW
	if (gtk_window_get_resizable(window))
		gtk_window_resize(window,
			g_settings_get_int(data->settings, EINA_DOCK_WINDOW_W_KEY),
			g_settings_get_int(data->settings, EINA_DOCK_WINDOW_H_KEY));
	g_signal_connect(data->dock, "activate", (GCallback) dock_activate_cb, data);
	#endif

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

static gboolean
dock_configure_event_cb(EinaDock *widget, GdkEventConfigure *ev, DockData *self)
{
	if (ev->type != GDK_CONFIGURE)
		return FALSE;
	
	GtkAllocation alloc;
	gtk_widget_get_allocation((GtkWidget *) widget, &alloc);
	g_settings_set_int(self->settings, EINA_DOCK_WINDOW_W_KEY, alloc.width);
	g_settings_set_int(self->settings, EINA_DOCK_WINDOW_H_KEY, alloc.height);
	g_debug("Saved allocation of %dx%d", alloc.width, alloc.height);
	return FALSE;
}

static void
dock_notify_cb(EinaDock *widget, GParamSpec *pspec, DockData *self)
{
	if (g_str_equal("resizable", pspec->name))
	{
		if (eina_dock_get_resizable(widget))
		{
			gtk_window_resize((GtkWindow *) gtk_widget_get_toplevel((GtkWidget *) widget),
				g_settings_get_int(self->settings, EINA_DOCK_WINDOW_W_KEY),
				g_settings_get_int(self->settings, EINA_DOCK_WINDOW_H_KEY)
				);
		}
	}
	else
		g_warning(_("Unhanded notify::%s"), pspec->name);
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
