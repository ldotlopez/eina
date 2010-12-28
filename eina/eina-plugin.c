#include "eina-plugin.h"

guint
eina_plugin_window_ui_manager_add_from_string(EinaPlugin *plugin, const gchar *ui_mng_str)
{
	g_return_val_if_fail(EINA_IS_PLUGIN(plugin), 0);
	g_return_val_if_fail(ui_mng_str != NULL, 0);

	EinaApplication *app = eina_plugin_get_application(plugin);
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);

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
eina_plugin_window_action_group_add_toggle_actions(EinaPlugin *plugin, const GtkToggleActionEntry *entries, guint n_entries)
{
	g_return_if_fail(EINA_IS_PLUGIN(plugin));
	g_return_if_fail(entries != NULL);
	g_return_if_fail(n_entries > 0);

	EinaApplication *app = eina_plugin_get_application(plugin);
	GtkActionGroup *ag = eina_application_get_window_action_group(app);

	gtk_action_group_add_toggle_actions(ag, entries, n_entries, plugin);
}

