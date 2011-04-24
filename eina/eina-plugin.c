/*
 * eina/eina-plugin.c
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

#include "eina-plugin.h"

guint
eina_plugin_window_ui_manager_add_from_string(EinaPlugin *plugin, const gchar *ui_mng_str)
{
	g_return_val_if_fail(EINA_IS_PLUGIN(plugin), 0);
	g_return_val_if_fail(ui_mng_str != NULL, 0);

	EinaApplication *app = eina_plugin_get_application(plugin);
	g_return_val_if_fail(EINA_IS_APPLICATION(app), 0);

	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
	g_return_val_if_fail(GTK_IS_UI_MANAGER(ui_mng), 0);

	GError *error = NULL;
	guint ret = gtk_ui_manager_add_ui_from_string(ui_mng, ui_mng_str, -1, &error);

	if (ret == 0)
	{
		g_warning(_("[%s] cannot add UI manager string: %s"), __FUNCTION__, error->message);
		g_error_free(error);
	}

	return ret;
}

void
eina_plugin_window_ui_manager_remove(EinaPlugin *plugin, guint id)
{
	g_return_if_fail(EINA_IS_PLUGIN(plugin));
	g_return_if_fail(id > 0);

	EinaApplication *app = eina_plugin_get_application(plugin);
	g_return_if_fail(EINA_IS_APPLICATION(app));

	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
	g_return_if_fail(GTK_IS_UI_MANAGER(ui_mng));

	gtk_ui_manager_remove_ui(ui_mng, id);
}

void     
eina_plugin_window_action_group_add_toggle_actions(EinaPlugin *plugin, const GtkToggleActionEntry *entries, guint n_entries)
{
	g_return_if_fail(EINA_IS_PLUGIN(plugin));
	g_return_if_fail(entries != NULL);
	g_return_if_fail(n_entries > 0);

	EinaApplication *app = eina_plugin_get_application(plugin);
	g_return_if_fail(EINA_IS_APPLICATION(app));

	GtkActionGroup *ag = eina_application_get_window_action_group(app);
	g_return_if_fail(GTK_IS_ACTION_GROUP(ag));

	gtk_action_group_add_toggle_actions(ag, entries, n_entries, plugin);
}

void
eina_plugin_window_action_group_remove_toogle_actions(EinaPlugin *plugin, const GtkToggleActionEntry *entries, guint n_entries)
{
	g_return_if_fail(EINA_IS_PLUGIN(plugin));
	g_return_if_fail(entries != NULL);
	g_return_if_fail(n_entries > 0);

	EinaApplication *app = eina_plugin_get_application(plugin);
	g_return_if_fail(EINA_IS_APPLICATION(app));

	GtkActionGroup *ag = eina_application_get_window_action_group(app);
	g_return_if_fail(GTK_IS_ACTION_GROUP(ag));

	for (guint i = 0; i < n_entries; i++)
		gtk_action_group_remove_action(ag, GTK_ACTION(gtk_action_group_get_action(ag, entries[i].name)));
}

