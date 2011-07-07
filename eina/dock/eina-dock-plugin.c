/*
 * eina/dock/dock.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-dock-plugin.h"
#include <eina/ext/eina-extension.h>

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_DOCK_PLUGIN         (eina_dock_plugin_get_type ())
#define EINA_DOCK_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_DOCK_PLUGIN, EinaDockPlugin))
#define EINA_DOCK_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_DOCK_PLUGIN, EinaDockPlugin))
#define EINA_IS_DOCK_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_DOCK_PLUGIN))
#define EINA_IS_DOCK_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_DOCK_PLUGIN))
#define EINA_DOCK_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_DOCK_PLUGIN, EinaDockPluginClass))

typedef struct {
	GtkWidget *dock;
	gboolean prev_state, delayed_open;
} EinaDockPluginPrivate;

EINA_PLUGIN_REGISTER(EINA_TYPE_DOCK_PLUGIN, EinaDockPlugin, eina_dock_plugin)

static void
dock_widget_add_cb(EinaDock *dock, const gchar *name, EinaDockPlugin *plugin);
static void
dock_notify_cb(EinaDock *dock, GParamSpec *pspec, EinaDockPlugin *plugin);

static gboolean
eina_dock_plugin_activate(EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(app), FALSE);
	EinaDockPluginPrivate *priv = EINA_DOCK_PLUGIN(plugin)->priv;

	// Setup dock
	priv->dock = (GtkWidget *) g_object_ref(eina_dock_new());
	priv->prev_state = eina_dock_get_resizable(EINA_DOCK(priv->dock));

	// Insert into applicatin
	GtkWindow *window = GTK_WINDOW(eina_application_get_window(app));
	gtk_container_add(GTK_CONTAINER(window), priv->dock);

	// Settings
	GSettings *settings = eina_application_get_settings(app, EINA_DOCK_PREFERENCES_DOMAIN);

	eina_dock_set_page_order((EinaDock *) priv->dock, (gchar **) g_settings_get_strv (settings, EINA_DOCK_ORDER_KEY));
	g_settings_bind (settings, EINA_DOCK_ORDER_KEY, priv->dock, "page-order", G_SETTINGS_BIND_DEFAULT);

	priv->delayed_open = g_settings_get_boolean(settings, EINA_DOCK_EXPANDED_KEY);
	g_settings_bind(settings, EINA_DOCK_EXPANDED_KEY, priv->dock, "resizable", G_SETTINGS_BIND_DEFAULT);

	g_object_bind_property(priv->dock, "resizable", window, "resizable", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

	g_signal_connect(priv->dock, "notify::resizable", (GCallback) dock_notify_cb,     plugin);
	g_signal_connect(priv->dock, "widget-add",        (GCallback) dock_widget_add_cb, plugin);

	// Run
	gtk_widget_show(priv->dock);
	eina_application_set_interface(app, "dock", priv->dock);

	return TRUE;
}

static gboolean
eina_dock_plugin_deactivate(EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaDockPluginPrivate *priv = EINA_DOCK_PLUGIN(plugin)->priv;

	g_return_val_if_fail(EINA_IS_DOCK(priv->dock), FALSE);

	GtkContainer *container = (GtkContainer *) gtk_widget_get_parent(priv->dock);
	g_return_val_if_fail(container && GTK_IS_CONTAINER(container), FALSE);

	GtkWindow *window = (GtkWindow *) gtk_widget_get_toplevel(priv->dock);
	g_return_val_if_fail(window && GTK_IS_WINDOW(window), FALSE);

	eina_application_set_interface(app, "dock", NULL);

	gtk_container_remove(container, priv->dock);

	g_object_unref(priv->dock);

	return TRUE;
}

/**
 * eina_application_get_dock:
 * @application: An #EinaApplication
 *
 * Gets the #EinaDock from #EinaApplication
 *
 * Returns: (transfer none): The #EinaDock
 */ 
EinaDock *eina_application_get_dock(EinaApplication *application)
{
	return (EinaDock *) eina_application_get_interface(application, "dock");
}

/*
 * Internal API
 */
static void 
save_size(EinaDockPlugin *plugin)
{
	g_return_if_fail(EINA_IS_DOCK_PLUGIN(plugin));
	EinaDockPluginPrivate *priv = plugin->priv;

	GSettings *settings = eina_application_get_settings(
		eina_activatable_get_application (EINA_ACTIVATABLE(plugin)),
		EINA_DOCK_PREFERENCES_DOMAIN);
	g_return_if_fail (G_IS_SETTINGS(settings));

	GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(priv->dock));
	g_return_if_fail(window != NULL);

	gint w, h;
	gtk_window_get_size(window, &w, &h);

	g_settings_set_int(settings, EINA_DOCK_WINDOW_W_KEY, w);
	g_settings_set_int(settings, EINA_DOCK_WINDOW_H_KEY, h);
}

static void 
restore_size(EinaDockPlugin *plugin)
{
	g_return_if_fail(EINA_IS_DOCK_PLUGIN(plugin));
	EinaDockPluginPrivate *priv = plugin->priv;

	GSettings *settings = eina_application_get_settings(
		eina_activatable_get_application (EINA_ACTIVATABLE(plugin)),
		EINA_DOCK_PREFERENCES_DOMAIN);
	g_return_if_fail (G_IS_SETTINGS(settings));

	GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(priv->dock));
	g_return_if_fail(GTK_IS_WINDOW(window));

	gtk_window_resize(window,
		g_settings_get_int(settings, EINA_DOCK_WINDOW_W_KEY),
		g_settings_get_int(settings, EINA_DOCK_WINDOW_H_KEY));
}

/**
 * eina_application_add_dock_widget:
 * @application: An #EinaApplication
 * @id: Unique ID to identify the widget
 * @widget: Widget to add
 * @label: Widget to use as label
 * @flags: Flags
 *
 * Adds the @widget into the application's docks
 *
 * Returns: (transfer none): The #EinaDockTab
 */
EinaDockTab*
eina_application_add_dock_widget(EinaApplication *application, const gchar *id, GtkWidget *widget, GtkWidget *label, EinaDockFlags flags)
{
	EinaDock *dock = eina_application_get_dock(application);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);

	return eina_dock_add_widget(dock, id, widget, label, flags);
}

gboolean
eina_application_switch_dock_widget(EinaApplication *application, EinaDockTab *tab)
{
	EinaDock *dock = eina_application_get_dock(application);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);

	return eina_dock_switch_widget(dock, tab);
}

gboolean
eina_application_remove_dock_widget(EinaApplication *application, EinaDockTab *tab)
{
	EinaDock *dock = eina_application_get_dock(application);
	g_return_val_if_fail(EINA_IS_DOCK(dock), FALSE);

	return eina_dock_remove_widget(dock, tab);
}

/*
 * Event callbacks
 */
static void
dock_widget_add_cb(EinaDock *dock, const gchar *name, EinaDockPlugin *plugin)
{
	EinaDockPluginPrivate *priv = EINA_DOCK_PLUGIN(plugin)->priv;

	if ((eina_dock_get_n_widgets(dock) == 2) && priv->delayed_open)
	{
		eina_dock_set_resizable(dock, TRUE);
		priv->delayed_open = FALSE;
	}
}

static void
dock_notify_cb(EinaDock *dock, GParamSpec *pspec, EinaDockPlugin *plugin)
{
	EinaDockPluginPrivate *priv = EINA_DOCK_PLUGIN(plugin)->priv;

	if (g_str_equal(pspec->name, "resizable"))
	{
		gboolean resizable = eina_dock_get_resizable(dock);
		if (resizable == priv->prev_state)
			return;

		if (resizable == TRUE)
			restore_size(plugin);
		else
			save_size(plugin);

		priv->prev_state = resizable;
	}
}

