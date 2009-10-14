/*
 * eina/eina-ige.c
 *
 * Copyright (C) 2004-2009 Eina
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

#define GEL_DOMAIN "Eina::EinaIge"
#define EINA_PLUGIN_DATA_TYPE EinaIge

#include <config.h>
#include <eina/eina-plugin.h>
#include <eina/window.h>
#include <igemacintegration/ige-mac-integration.h>

typedef struct {
	IgeMacDock *dock;
	IgeMacMenuGroup *menu;
} EinaIge;

enum {
	EINA_IGE_NO_ERROR = 0,
	EINA_IGE_PLAYER_NOT_LOADED
};

static void
ui_manager_actions_changed_cb(GtkUIManager *ui_mng, EinaIge *self);

static gboolean
ige_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	EinaIge *self = g_new0(EinaIge, 1);

	// Build dock item
	if ((self->dock = ige_mac_dock_new()) != NULL)
	{
		gchar     *path = NULL;
		GdkPixbuf *pb   = NULL;
		GError    *err  = NULL;

		if ((path = gel_plugin_get_resource(plugin, GEL_RESOURCE_IMAGE, "eina.svg")) != NULL)
			pb = gdk_pixbuf_new_from_file_at_size(path, 512, 512, &err);
		
		if (!path)
			gel_warn(N_("Cannot locate %s"), "eina.svg");
		else if (!pb)
		{
			gel_warn(N_("Cannot load '%s': '%s'"), path, err->message);
			g_error_free(err);
		}
		else
			ige_mac_dock_set_icon_from_pixbuf(self->dock, pb);

		gel_free_and_invalidate(path, NULL, g_free);
	}

	// Build menu
	GtkUIManager *ui_mng = eina_window_get_ui_manager(GEL_APP_GET_WINDOW(app));
	g_signal_connect((GObject *) ui_mng, "actions-changed", (GCallback) ui_manager_actions_changed_cb, self);

	GtkWidget *main_menu_bar = gtk_ui_manager_get_widget(ui_mng, "/MainMenuBar");
	gtk_widget_hide(main_menu_bar);

	ige_mac_menu_set_menu_bar(GTK_MENU_SHELL(main_menu_bar));
	ige_mac_menu_set_quit_menu_item(GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui_mng, "/MainMenuBar/File/Quit")));

	IgeMacMenuGroup *group;

	group = ige_mac_menu_add_app_menu_group ();
	ige_mac_menu_add_app_menu_item(group,
		GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui_mng, "/MainMenuBar/Help/About")),
		NULL);

	group = ige_mac_menu_add_app_menu_group ();
		ige_mac_menu_add_app_menu_item  (group,
		GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui_mng, "/MainMenuBar/Edit/Preferences")),
		NULL);

	gel_warn(gtk_ui_manager_get_ui(ui_mng));
	plugin->data = self;

	return TRUE;
}

static gboolean
ige_plugin_exit(GelApp *app, EinaPlugin *plugin, GError **error)
{
	EinaIge *self = EINA_PLUGIN_DATA(plugin);
	g_object_unref(self->dock);
	g_free(self);
	return TRUE;
}

static void
ui_manager_actions_changed_cb(GtkUIManager *ui_mng, EinaIge *self)
{
/*
	enum {
		EINA_IGE_ABOUT_ITEM,
		EINA_IGE_QUIT_ITEM,
		EINA_IGE_PREFERENCES_ITEM
	} EinaIgeItem; */
	gchar *paths[]= {
		"/MainMenuBar/Help/About",
		"/MainMenuBar/File/Quit",
		"/MainMenuBar/Edit/Preferences",
	};

	gint i;
	for (i = 0; i < G_N_ELEMENTS(paths); i++)
	{
		GtkWidget *widget = gtk_ui_manager_get_widget(ui_mng, paths[i]);
		if (widget)
			gel_warn("Found %s", paths[i]);
	}
}

G_MODULE_EXPORT EinaPlugin ige_plugin = {
	EINA_PLUGIN_SERIAL,
	"ige", PACKAGE_VERSION, "window,player,preferences",
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,

	N_("OSX integration"), NULL, NULL,

	ige_plugin_init, ige_plugin_exit,

	NULL, NULL, NULL
};

