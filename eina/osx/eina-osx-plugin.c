/*
 * eina/osx/eina-osx-plugin.c
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

#include <eina/core/eina-extension.h>
#include <gel/gel.h>
#include <gtkosxapplication.h>

#define EINA_TYPE_OSX_PLUGIN         (eina_osx_plugin_get_type ())
#define EINA_OSX_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_OSX_PLUGIN, EinaOsxPlugin))
#define EINA_OSX_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k),    EINA_TYPE_OSX_PLUGIN, EinaOsxPlugin))
#define EINA_IS_OSX_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_OSX_PLUGIN))
#define EINA_IS_OSX_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_OSX_PLUGIN))
#define EINA_OSX_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_OSX_PLUGIN, EinaOsxPluginClass))

typedef struct {
	GtkOSXApplication *app;
} EinaOsxPluginPrivate;
EINA_PLUGIN_REGISTER (EINA_TYPE_OSX_PLUGIN, EinaOsxPlugin, eina_osx_plugin)

static void
osx_app_open_files(EinaOsxPlugin *plugin, gchar **paths);

static gboolean
eina_osx_plugin_activate (EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaOsxPlugin      *plugin = EINA_OSX_PLUGIN (activatable);
	EinaOsxPluginPrivate *priv = plugin->priv;

	priv->app = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
	gtk_osxapplication_set_use_quartz_accelerators (priv->app, TRUE);
	gtk_osxapplication_ready (priv->app);

	GtkUIManager *uimng = eina_application_get_window_ui_manager (app);
	GtkMenuShell *ms = GTK_MENU_SHELL (gtk_ui_manager_get_widget (uimng, "/Main"));
	gtk_widget_hide ((GtkWidget *) ms);
	gtk_osxapplication_set_menu_bar (priv->app, ms);
	gtk_osxapplication_sync_menubar (priv->app);

	gtk_widget_hide (GTK_WIDGET (gtk_ui_manager_get_widget (uimng, "/Main/File/Quit")));

	gtk_osxapplication_set_help_menu(priv->app,
		GTK_MENU_ITEM (gtk_ui_manager_get_widget (uimng, "/Main/Help")));

	gtk_osxapplication_insert_app_menu_item(priv->app,
		gtk_ui_manager_get_widget (uimng, "/Main/Help/About"),
		0);

	gtk_osxapplication_insert_app_menu_item(priv->app,
		gtk_ui_manager_get_widget (uimng, "/Main/Edit/Preferences"),
		1);

	gtk_osxapplication_set_dock_menu (priv->app,
		GTK_MENU_SHELL (gtk_ui_manager_get_widget (uimng, "/Main")));

	g_signal_connect(priv->app, "NSApplicationOpenFiles", (GCallback) osx_app_open_files, plugin);

	return TRUE;
}

static gboolean
eina_osx_plugin_deactivate (EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaOsxPlugin      *plugin = EINA_OSX_PLUGIN (activatable);
	EinaOsxPluginPrivate *priv = plugin->priv;

	gel_free_and_invalidate (priv->app, NULL, g_object_unref);  

	return TRUE;
}

static void
osx_app_open_files(EinaOsxPlugin *plugin, gchar **paths)
{
	for (guint i = 0; paths && paths[i]; i++)
		g_warning("Should load: '%s'", paths[i]);
}

