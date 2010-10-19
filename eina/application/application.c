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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "application.h"

#define BORDER_WIDTH 5

typedef struct {
	guint ui_mng_merge_id;
	GtkActionGroup *ag;
} ApplicationData;

static void
action_activate_cb(GtkAction *action, GelPluginEngine *engine);

static gchar *ui_mng_xml =
"<ui>"
"  <menubar name='Main'>"
"    <menu name='Edit' action='edit-menu' >"
"      <menuitem name='PluginManager' action='plugin-manager-action' />"
"    </menu>"
"  </menubar>"
"</ui>";

static GtkActionEntry ui_mng_actions[] = {
	{ "edit-menu",          NULL,              N_("_Edit"),        "<alt>e",     NULL, NULL},
	{ "plugin-manager-action", EINA_STOCK_PLUGIN, N_("Select pl_ugins"), "<control>u", NULL, (GCallback) action_activate_cb }
};


static gboolean
window_delete_event_cb(GtkWidget *w, GdkEvent *ev, GtkApplication *app)
{
	gtk_widget_hide(w);
	gtk_application_quit(GTK_APPLICATION(app));
	return TRUE;
}

G_MODULE_EXPORT gboolean
application_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	ApplicationData *data = g_new0(ApplicationData, 1);
	gel_plugin_set_data(plugin, data);

	GelUIApplication *application = gel_ui_application_new(
		EINA_DOMAIN,
		gel_plugin_engine_get_argc(engine),
		gel_plugin_engine_get_argv(engine));
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(application), FALSE);

	// Generate iconlist
	const gint sizes[] = { 16, 32, 48, 64, 128 };
	GList *icon_list = NULL;
	gchar *icon_filename = gel_resource_locate(GEL_RESOURCE_IMAGE, "eina.svg");
	if (icon_filename)
	{
		GError *e = NULL;
		GdkPixbuf *pb = NULL;
		for (guint i = 0; i < G_N_ELEMENTS(sizes); i++)
		{
			if (!(pb = gdk_pixbuf_new_from_file_at_scale(icon_filename, sizes[i], sizes[i], TRUE, &e)))
			{
				g_warning(N_("Unable to load resource '%s': %s"), icon_filename, e->message);
				break;
			}
			else
				icon_list = g_list_prepend(icon_list, pb);
		}
		gel_free_and_invalidate(e, NULL, g_error_free);

		gtk_window_set_default_icon_list(icon_list);
	 	gel_list_deep_free(icon_list, g_object_unref);
	}
	else
		g_warning(N_("Unable to locate resource '%s'"), "eina.svg");

	// Setup window
	GtkWindow *w = GTK_WINDOW(gel_ui_application_get_window (application));
	g_object_set((GObject *) w,
		"title", N_("Eina music player"),
		"resizable", FALSE,
		NULL);
	g_signal_connect(w, "delete-event", (GCallback) window_delete_event_cb, application);

	GtkContainer *content_area = GTK_CONTAINER(gel_ui_application_get_window_content_area(application));
	gtk_container_set_border_width(content_area, BORDER_WIDTH);

    // Attach menus
	GtkUIManager *ui_mng = gel_ui_application_get_window_ui_manager(application);
	GError *e = NULL;
	if ((data->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string (ui_mng, ui_mng_xml, -1 , &e)) == 0)
	{
		g_warning(N_("Unable to add UI to window GtkUIManager: '%s'"), e->message);
		g_error_free(e);
	}
	else
	{
		data->ag = gtk_action_group_new("application");
		gtk_action_group_add_actions(data->ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), engine);
		gtk_ui_manager_insert_action_group(ui_mng, data->ag, G_MAXINT);
	}

	// Show and publish
	gtk_widget_show_all ((GtkWidget *) w);
	gel_plugin_engine_set_interface(engine, "application", application);

	return TRUE;
}

G_MODULE_EXPORT gboolean
application_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	// Unpublish
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	gel_plugin_engine_set_interface(engine, "application", NULL);

	ApplicationData *data = gel_plugin_get_data(plugin);

	// Deattach menus
	GtkUIManager *ui_mng = gel_ui_application_get_window_ui_manager(application);
	gtk_ui_manager_remove_ui(ui_mng, data->ui_mng_merge_id);
	gtk_ui_manager_remove_action_group(ui_mng, data->ag);
	g_object_unref(data->ag);

	// Destroy application and free plugin resources
	g_object_unref(application);

	gel_plugin_set_data(plugin, NULL);
	g_free(data);
	
	return TRUE;
}

static void 
action_activate_cb(GtkAction *action, GelPluginEngine *engine)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal ("plugin-manager-action", name))
	{
		GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
		g_return_if_fail(GEL_UI_IS_APPLICATION(application));

		GtkDialog *dialog = (GtkDialog *) gtk_dialog_new_with_buttons(N_("Select plugins"),
			gel_ui_application_get_window ((GelUIApplication *) application),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);

		gtk_container_add(
			(GtkContainer *) gtk_dialog_get_content_area (dialog),
			(GtkWidget    *) gel_ui_plugin_manager_new(engine));
		gint w, h;
		gtk_window_get_size(gel_ui_application_get_window(application),&w ,&h);
		gtk_window_resize((GtkWindow *) dialog, w * 3 / 4 , h * 3 / 4);

		gtk_widget_show_all((GtkWidget *) dialog);
		gtk_dialog_run(dialog);
		gtk_widget_destroy((GtkWidget *) dialog);
	}
}

