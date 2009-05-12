/*
 * eina/preferences.c
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

#define GEL_DOMAIN "Eina::Preferences"

#include <config.h>
#include <eina/player.h>
#include <eina/eina-preferences-dialog.h>
#include <eina/preferences.h>

struct  _EinaPreferences {
	EinaObj parent;

	EinaPreferencesDialog *dialog;
	guint n_widgets;

	GtkActionGroup *ag;
	guint ui_merge_id;
};

static void
attach_menu(EinaPreferences *self);
static void
deattach_menu(EinaPreferences *self);
static void
menu_activate_cb(GtkAction *action, EinaPreferences *self);

enum {
	EINA_PREFERENCES_NO_ERROR = 0,
	EINA_PREFERENCES_CANNOT_ACCESS_PLAYER,
	EINA_PREFERENCES_CANNOT_ACCESS_UI_MANAGER,
	EINA_PREFERENCES_CANNOT_REGISTER_SHARED
};
/*
static GQuark
preferences_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("preferences");
	return ret;
}
*/
static gboolean
preferences_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences       *self;

	self = g_new0(EinaPreferences, 1);
	if (!eina_obj_init((EinaObj*) self, app, "preferences", EINA_OBJ_NONE, error))
	{
		g_free(self);
		return FALSE;
	}
	plugin->data = self;

	// Setup dialog
	self->dialog = eina_preferences_dialog_new();
	g_signal_connect(G_OBJECT(self->dialog), "response", G_CALLBACK(gtk_widget_hide), self->dialog);
	g_signal_connect(G_OBJECT(self->dialog), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), self->dialog);

	return TRUE;
}

static gboolean preferences_fini
(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences *self = EINA_PREFERENCES(plugin->data);

	if (self->n_widgets > 0)
		deattach_menu(self);

	gtk_widget_destroy((GtkWidget *) self->dialog);
	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

void
eina_preferences_add_tab(EinaPreferences *self, GtkImage *icon, GtkLabel *label, GtkWidget *tab)
{
	eina_preferences_dialog_add_tab(self->dialog, icon, label, tab);
	self->n_widgets++;
	if (self->n_widgets == 1)
		attach_menu(self);
}

void
eina_preferences_remove_tab(EinaPreferences *self, GtkWidget *tab)
{
	eina_preferences_dialog_remove_tab(self->dialog, tab);
	self->n_widgets--;
	if (self->n_widgets == 0)
		deattach_menu(self);
}

static void
attach_menu(EinaPreferences *self)
{
	const GtkActionEntry action_entries[] = {
		{ "Preferences", GTK_STOCK_PREFERENCES, N_("Preferences"),
		"<Control>p", NULL, G_CALLBACK(menu_activate_cb) },
	};

	const gchar *ui_xml = 
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Preferences\" action=\"Preferences\" />"
		"</menu>"
		"</menubar>"
		"</ui>";
	EinaPlayer *player = EINA_OBJ_GET_PLAYER(self);
	if (player == NULL)
	{
		gel_error("Cannot get player reference, unable to attach preferences menu");
		return;
	}

	GtkUIManager *ui_manager = eina_player_get_ui_manager(player);
	if (ui_manager == NULL)
	{
		gel_error("Cannot get GtkUIManager for main menu, unable to attach preferences menu");
		return;
	}

	GError *err = NULL;
	if ((self->ui_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager, ui_xml, -1, &err)) == 0)
	{
		gel_error("Cannot attach menu: %s", err->message);
		g_error_free(err);
	}

	self->ag = gtk_action_group_new("preferences");
	gtk_action_group_add_actions(self->ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(ui_manager, self->ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);
}

static void
deattach_menu(EinaPreferences *self)
{
	EinaPlayer *player = EINA_OBJ_GET_PLAYER(self);
	if (player == NULL)
	{
		gel_error("Cannot get player reference, unable to deattach preferences menu");
		return;
	}

	GtkUIManager *ui_manager = eina_player_get_ui_manager(player);
	if (ui_manager == NULL)
	{
		gel_error("Cannot get GtkUIManager for main menu, unable to deattach preferences menu");
		return;
	}

	gtk_ui_manager_remove_action_group(ui_manager, self->ag);
	g_object_unref(self->ag);

	gtk_ui_manager_remove_ui(ui_manager, self->ui_merge_id);
}

static void
menu_activate_cb(GtkAction *action, EinaPreferences *self)
{
	if (g_str_equal(gtk_action_get_name(action), "Preferences"))
	{
		gtk_dialog_run(GTK_DIALOG(self->dialog));
	}
}

G_MODULE_EXPORT GelPlugin preferences_plugin = {
	GEL_PLUGIN_SERIAL,
	"preferences", PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Build-in preferences plugin"), NULL, NULL,

	preferences_init, preferences_fini,

	NULL, NULL, NULL
};

