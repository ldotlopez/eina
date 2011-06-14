/*
 * eina/muine/eina-muine-plugin.c
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

#include "eina-muine-plugin.h"
#include <eina/ext/eina-extension.h>
#include <eina/adb/eina-adb-plugin.h>
#include <eina/dock/eina-dock-plugin.h>
#include <eina/lomo/eina-lomo-plugin.h>

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_MUINE_PLUGIN         (eina_muine_plugin_get_type ())
#define EINA_MUINE_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_MUINE_PLUGIN, EinaMuinePlugin))
#define EINA_MUINE_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_MUINE_PLUGIN, EinaMuinePlugin))
#define EINA_IS_MUINE_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_MUINE_PLUGIN))
#define EINA_IS_MUINE_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_MUINE_PLUGIN))
#define EINA_MUINE_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_MUINE_PLUGIN, EinaMuinePluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaMuinePlugin, eina_muine_plugin)
EINA_DEFINE_EXTENSION(EinaMuinePlugin, eina_muine_plugin, EINA_TYPE_MUINE_PLUGIN)

static gboolean
eina_muine_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaMuine *d = eina_muine_new();
	g_object_set((GObject *) d,
		"adb",         eina_application_get_adb(app),
		"lomo-player", eina_application_get_lomo(app),
		NULL);

	GSettings *settings = eina_application_get_settings(app, EINA_MUINE_PREFERENCES_DOMAIN);
	g_settings_bind(settings, EINA_MUINE_GROUP_BY_KEY, d, "mode", G_SETTINGS_BIND_DEFAULT);

	EinaDockTab *tab = eina_application_add_dock_widget(app,
		N_("Muine"),
		(GtkWidget *) g_object_ref(d),
		gtk_image_new_from_stock("gtk-dnd-multiple", GTK_ICON_SIZE_MENU),
		EINA_DOCK_DEFAULT);
	g_object_set_data((GObject *) d, "x-muine-dock-tab", tab);

	eina_activatable_set_data(activatable, d);

	return TRUE;
}

static gboolean
eina_muine_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaMuine *d = eina_activatable_steal_data(activatable);

	EinaDockTab *tab = (EinaDockTab *) g_object_get_data((GObject *) d, "x-muine-dock-tab");
	eina_application_remove_dock_widget(app, tab);

	g_object_unref(d);

	return TRUE;
}

