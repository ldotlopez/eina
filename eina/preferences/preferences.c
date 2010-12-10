/*
 * eina/preferences/preferences.c
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

#include "preferences.h"

struct  _EinaPreferences {
	GelPlugin *plugin;

	EinaPreferencesDialog *dialog;
	GList *tabs;

	GtkActionGroup *ag;
	guint ui_merge_id;
};

GEL_DEFINE_WEAK_REF_CALLBACK(preferences)

static void
preferences_attach_menu(EinaPreferences *self);
static void
preferences_deattach_menu(EinaPreferences *self);
static void 
preferences_deattach_all_tabs(EinaPreferences *self);
static void
menu_activate_cb(GtkAction *action, EinaPreferences *self);

// Dialog
static void
response_cb(GtkWidget *w, gint response, EinaPreferences *self);
static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferences *self);

enum {
	EINA_PREFERENCES_NO_ERROR = 0,
	EINA_PREFERENCES_CANNOT_ACCESS_PLAYER,
	EINA_PREFERENCES_CANNOT_ACCESS_UI_MANAGER,
	EINA_PREFERENCES_CANNOT_REGISTER_SHARED
};

G_MODULE_EXPORT gboolean
preferences_plugin_init (EinaApplication *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences *self = g_new0(EinaPreferences, 1);
	self->plugin = plugin;
	gel_plugin_set_data(plugin, self);
	eina_application_set_interface(app, "preferences", self);

	return TRUE;
}

G_MODULE_EXPORT gboolean
preferences_plugin_fini (EinaApplication *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences *self = (EinaPreferences *) gel_plugin_steal_data(plugin);

	if (self->dialog)
	{
		preferences_deattach_all_tabs(self);
		gtk_widget_destroy((GtkWidget *) self->dialog);
		self->dialog = NULL;
	}

	if (self->tabs)
	{
		GList *iter = self->tabs;
		while (iter)
		{
			eina_preferences_remove_tab(self, EINA_PREFERENCES_TAB(iter->data));
			iter = self->tabs;
		}
		g_list_free(self->tabs);
		self->tabs = NULL;
		// Edit/Preferences menu is deattached automatically after remove last tab
	}


	g_free(self);
	return TRUE;
}

void
eina_preferences_add_tab(EinaPreferences *self, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));
	g_return_if_fail(g_list_find(self->tabs, tab) == NULL);

	self->tabs = g_list_prepend(self->tabs, g_object_ref(tab)); // owning
	g_object_weak_ref((GObject *) tab, (GWeakNotify) preferences_weak_ref_cb, NULL);

	if (self->dialog)
		eina_preferences_dialog_add_tab(self->dialog, tab);

	// Just one tab
	if (self->tabs && !self->tabs->next)
		preferences_attach_menu(self);
}

void
eina_preferences_remove_tab(EinaPreferences *self, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));
	g_return_if_fail(g_list_find(self->tabs, tab) != NULL);

	if (self->dialog)
		eina_preferences_dialog_remove_tab(self->dialog, tab);

	g_object_weak_unref((GObject *) tab, (GWeakNotify) preferences_weak_ref_cb, NULL);
	self->tabs = g_list_remove(self->tabs, tab);

	g_object_unref(G_OBJECT(tab));

	// No tabs
	if (!self->tabs)
		preferences_deattach_menu(self);
}

static void
build_dialog(EinaPreferences *self)
{
	self->dialog = eina_preferences_dialog_new();
	g_object_set((GObject*) self->dialog,
		"title", N_("Preferences"),
		"window-position", GTK_WIN_POS_CENTER_ON_PARENT,
		"width-request",  600,
		"height-request", 400,
		NULL);
	g_signal_connect(self->dialog, "response",     (GCallback) response_cb, self);
	g_signal_connect(self->dialog, "delete-event", (GCallback) delete_event_cb, self);

	GList *iter = g_list_last(self->tabs);
	while (iter)
	{
		eina_preferences_dialog_add_tab(self->dialog, EINA_PREFERENCES_TAB(iter->data));
		iter = iter->prev;
	}
}

static void
preferences_attach_menu(EinaPreferences *self)
{
	const GtkActionEntry action_entries[] = {
		{ "preferences", GTK_STOCK_PREFERENCES, N_("Preferences"), "<Control>p", NULL, G_CALLBACK(menu_activate_cb) },
	};

	const gchar *ui_xml = 
		"<ui>"
		"<menubar name=\"Main\">"
		"<menu name=\"Edit\" action=\"edit-menu\">"
		"  <menuitem name=\"Preferences\" action=\"preferences\" />"
		"</menu>"
		"</menubar>"
		"</ui>";

	EinaApplication *app = eina_plugin_get_application(self->plugin);
	GtkUIManager *ui_manager = eina_application_get_window_ui_manager(app);

	if (ui_manager == NULL)
	{
		g_warning(N_("Cannot get GtkUIManager for main menu, unable to attach preferences menu"));
		return;
	}

	GError *err = NULL;
	if ((self->ui_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager, ui_xml, -1, &err)) == 0)
	{
		g_warning(N_("Cannot attach menu: %s"), err->message);
		g_error_free(err);
	}

	self->ag = gtk_action_group_new("preferences");
	gtk_action_group_add_actions(self->ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(ui_manager, self->ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);
}

static void
preferences_deattach_menu(EinaPreferences *self)
{
	EinaApplication *app = eina_plugin_get_application(self->plugin);
	GtkUIManager *ui_manager = eina_application_get_window_ui_manager(app);
	if (ui_manager == NULL)
	{
		g_warning(N_("Cannot get GtkUIManager for main menu, unable to deattach preferences menu"));
		return;
	}

	g_warn_if_fail(self->ui_merge_id > 0);
	if (self->ui_merge_id)
	{
		gtk_ui_manager_remove_ui(ui_manager, self->ui_merge_id);
		self->ui_merge_id = 0;
	}

	g_warn_if_fail(self->ag != NULL);
	if (self->ag)
	{
		gtk_ui_manager_remove_action_group(ui_manager, self->ag);
		g_object_unref(self->ag);
		self->ag = NULL;
	}
}

static void
menu_activate_cb(GtkAction *action, EinaPreferences *self)
{
	if (g_str_equal(gtk_action_get_name(action), "preferences"))
	{
		build_dialog(self);
		gtk_dialog_run(GTK_DIALOG(self->dialog));
	}
}

static void 
preferences_deattach_all_tabs(EinaPreferences *self)
{
	g_warn_if_fail(self->dialog);
	if (self->dialog)
	{
		GList *iter = self->tabs;
		while (iter)
		{
			eina_preferences_dialog_remove_tab(self->dialog, EINA_PREFERENCES_TAB(iter->data));
			iter = iter->next;
		}
	}
}

static void
response_cb(GtkWidget *w, gint response, EinaPreferences *self)
{
	preferences_deattach_all_tabs(self);
	self->dialog = NULL;

	gtk_widget_destroy(w);
}

static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferences *self)
{
	preferences_deattach_all_tabs(self);
	self->dialog = NULL;

	return FALSE;
}

