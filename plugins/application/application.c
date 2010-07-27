/*
 * plugins/application/application.c
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

#include <config.h>
#include "eina/eina-plugin2.h"

static void
application_action_cb(GtkApplication *application, gchar *name, GelPluginEngine *engine)
{
	if (g_str_equal ("PluginManagerItem", name))
	{
		GtkDialog *dialog = (GtkDialog *) gtk_dialog_new_with_buttons(N_("Select plugins"),
			gel_ui_application_get_window ((GelUIApplication *) application),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);
	
		gtk_container_add(
			(GtkContainer *) gtk_dialog_get_content_area (dialog),
			(GtkWidget    *) gel_ui_plugin_manager_new(engine));
		gtk_widget_show_all((GtkWidget *) dialog);
		gtk_dialog_run(dialog);
		gtk_widget_destroy((GtkWidget *) dialog);
	}
}

G_MODULE_EXPORT gboolean
application_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GelUIApplication *application = gel_ui_application_new(
		EINA_DOMAIN,
		gel_plugin_engine_get_argc(engine),
		gel_plugin_engine_get_argv(engine));

	if (!application)
		return FALSE;

	g_signal_connect(application, "action", (GCallback) application_action_cb, engine);

	GtkWindow *w = GTK_WINDOW(gel_ui_application_get_window (application));
	gtk_widget_show_all ((GtkWidget *) w);
	gel_plugin_engine_set_interface(engine, "application", application);
	return TRUE;
}

G_MODULE_EXPORT gboolean
application_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	g_object_unref(application);

	return TRUE;
}

EINA_PLUGIN_INFO_SPEC(application,
	NULL,
	NULL,
	NULL,
	NULL,
	N_("Application services"),
	NULL,
	NULL
	);
