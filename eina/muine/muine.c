/*
 * plugins/muine/muine.c
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

#include "muine.h"
#include <eina/adb/adb.h>
#include <eina/application/application.h>
#include <eina/dock/dock.h>
#include <eina/lomo/lomo.h>

G_MODULE_EXPORT gboolean
muine_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaMuine *d = eina_muine_new();
	g_object_set((GObject *) d,
		"art",  eina_plugin_get_art(plugin),
		"adb",  eina_plugin_get_adb(plugin),
		"lomo-player", eina_plugin_get_lomo(plugin),
		NULL);
	gel_plugin_set_data(plugin, d);

	GSettings *settings = gel_ui_application_get_settings(eina_plugin_get_application(plugin), EINA_MUINE_PREFERENCES_DOMAIN);
	g_settings_bind(settings, EINA_MUINE_GROUP_BY_KEY, d, "mode", G_SETTINGS_BIND_DEFAULT);

	g_object_ref(d);

	eina_plugin_add_dock_widget(plugin, "muine", gtk_label_new(N_("Muine")), (GtkWidget *) d);
	return TRUE;
}

G_MODULE_EXPORT gboolean
muine_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	eina_plugin_remove_dock_widget(plugin, "muine");
	g_object_unref(gel_plugin_steal_data(plugin));

	return TRUE;
}


