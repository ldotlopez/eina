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
#include <eina/player.h>
#include <igemacintegration/ige-mac-integration.h>

typedef struct {
	IgeMacDock *dock;
	IgeMacMenuGroup *menu;
} EinaIge;

enum {
	EINA_IGE_NO_ERROR = 0,
	EINA_IGE_PLAYER_NOT_LOADED
};

static GQuark
ige_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-ige");
	return ret;
}

static gboolean
ige_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	if (gel_app_get_plugin_by_name(app, "player") == NULL)
	{
		g_set_error(error, ige_quark(), EINA_IGE_PLAYER_NOT_LOADED, N_("player plugin not loaded"));
		return FALSE;
	}
	EinaIge *self = g_new0(EinaIge, 1);

	gchar *path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "icon-512.png");

	self->dock = ige_mac_dock_new();
	ige_mac_dock_set_icon_from_pixbuf(self->dock, gdk_pixbuf_new_from_file(path, NULL));
	g_free(path);

	GtkUIManager *ui_mng = eina_player_get_ui_manager(GEL_APP_GET_PLAYER(app));
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

G_MODULE_EXPORT EinaPlugin ige_plugin = {
	EINA_PLUGIN_SERIAL,
	"ige", PACKAGE_VERSION,
	N_("OSX integration"), NULL, NULL,
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	ige_plugin_init, ige_plugin_exit,

	NULL, NULL
};

