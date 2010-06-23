/*
 * eina/eina-ige.c
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

#define GEL_DOMAIN "Eina::EinaIge"
#define EINA_PLUGIN_DATA_TYPE EinaIge

#include <config.h>
#include <eina/eina-plugin.h>
#include <eina/window.h>
#include <igemacintegration/ige-mac-integration.h>
#include <igemacintegration/gtkosxapplication.h>

typedef struct {
	EinaObj            parent;
	GtkOSXApplication *osxapp;
	// IgeMacDock      *dock;
	GtkOSXApplicationMenuGroup *about_gr, *prefs_gr;
	guint flags;
} EinaIge;

#define	ABOUT_MENU (1 << 0)
#define PREFS_MENU (1 << 1)

static void
ige_sync_menu(EinaIge *self, GtkUIManager *ui_mng);

G_MODULE_EXPORT gboolean
ige_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	EinaIge *self = g_new0(EinaIge, 1);
	if (!eina_obj_init((EinaObj *) self, plugin, "ige", EINA_OBJ_NONE, error))
		return FALSE;

	self->osxapp = g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
	gtk_osxapplication_set_use_quartz_accelerators(self->osxapp, TRUE);

	GtkUIManager *ui_mng        = eina_window_get_ui_manager(GEL_APP_GET_WINDOW(app));
	GtkWidget    *main_menu_bar = gtk_ui_manager_get_widget(ui_mng, "/Main");
	gtk_widget_hide(main_menu_bar);

	gtk_osxapplication_set_menu_bar(self->osxapp, GTK_MENU_SHELL(main_menu_bar));
	self->about_gr = gtk_osxapplication_add_app_menu_group(self->osxapp);
	self->prefs_gr = gtk_osxapplication_add_app_menu_group(self->osxapp);

	gtk_osxapplication_ready(self->osxapp);
	ige_sync_menu(self, NULL);

	g_signal_connect_swapped((GObject *) ui_mng, "actions-changed", (GCallback) ige_sync_menu, self);

	gel_plugin_set_data(plugin, self);
#if 0
	// Build dock item (in 10.5+ Cocoa apps automaticaly get icon from app
	// bundle, in addition 10.6+ deprecates Carbon access to dock, so ige dock
	// functions are not working anymore)
#if 0
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
#endif

	// Build menu
	self->about_gr = ige_mac_menu_add_app_menu_group ();
	self->prefs_gr = ige_mac_menu_add_app_menu_group ();

	GtkUIManager *ui_mng = eina_window_get_ui_manager(GEL_APP_GET_WINDOW(app));
	GtkWidget *main_menu_bar = gtk_ui_manager_get_widget(ui_mng, "/Main");
	gtk_widget_hide(main_menu_bar);

	ige_mac_menu_set_menu_bar(GTK_MENU_SHELL(main_menu_bar));

	g_signal_connect_swapped((GObject *) ui_mng, "actions-changed", (GCallback) ige_sync_menu, self);

	ige_sync_menu(self, NULL);

	gel_plugin_set_data(plugin, self);
#endif
	return TRUE;
}

G_MODULE_EXPORT gboolean
ige_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
#if 0
	EinaIge *self = EINA_PLUGIN_DATA(plugin);
	// g_object_unref(self->dock);
	eina_obj_fini((EinaObj *) self);
#endif
	return TRUE;
}

static void
ige_sync_menu(EinaIge *self, GtkUIManager *ui_mng)
{
	if (ui_mng == NULL)
		ui_mng = eina_window_get_ui_manager(eina_obj_get_window(self));

	g_return_if_fail(ui_mng != NULL);

	gchar *paths[]= {
		"/Main/Help/About",
		"/Main/File/Quit",
		"/Main/Edit/Preferences",
	};
	gint i;
	for (i = 0; i < G_N_ELEMENTS(paths); i++)
	{
		GtkWidget *widget = gtk_ui_manager_get_widget(ui_mng, paths[i]);
		if (widget == NULL)
			continue;

		if (g_str_equal(paths[i], "/Main/File/Quit"))
		{
			gtk_widget_hide(widget);
			ige_mac_menu_set_quit_menu_item((GtkMenuItem *) widget);
		}

		else if (g_str_equal(paths[i], "/Main/Help/About") && !(self->flags & ABOUT_MENU))
		{
			self->flags |= ABOUT_MENU;
			gtk_widget_hide(widget);
			gtk_osxapplication_add_app_menu_item(self->osxapp, self->about_gr, (GtkMenuItem *) widget);
		}
		else if (g_str_equal(paths[i], "/Main/Edit/Preferences") && !(self->flags & PREFS_MENU))
		{
			self->flags |= PREFS_MENU;
			gtk_widget_hide(widget);
			gtk_osxapplication_add_app_menu_item(self->osxapp, self->prefs_gr, (GtkMenuItem *) widget);
		}
	}
	gtk_osxapplication_sync_menubar(self->osxapp);
}

EINA_PLUGIN_INFO_SPEC(ige,
	PACKAGE_VERSION, "window",
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,

	N_("OSX integration"), NULL, NULL
);

