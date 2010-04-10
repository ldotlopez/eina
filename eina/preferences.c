/*
 * eina/preferences.c
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

#define GEL_DOMAIN "Eina::Preferences"

#include <config.h>
#include <eina/ext/eina-preferences-dialog.h>
#include <eina/eina-plugin.h>
#include <eina/player.h>

struct  _EinaPreferences {
	EinaObj parent;

	EinaPreferencesDialog *dialog;
	GList *tabs;
	guint n_tabs;

	GtkActionGroup *ag;
	guint ui_merge_id;

};

static void
preferences_attach_menu(EinaPreferences *self);
static void
preferences_deattach_menu(EinaPreferences *self);
static void 
preferences_remove_all_tabs(EinaPreferences *self);
static void
menu_activate_cb(GtkAction *action, EinaPreferences *self);

// Dialog
static void
response_cb(GtkWidget *w, gint response, EinaPreferences *self);
static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferences *self);
static void
value_changed_cb(EinaPreferencesDialog *w, gchar *key, GValue *value, EinaPreferences *self);


// conf


static void
change_cb(EinaConf *conf, gchar *key, EinaPreferences *self);

enum {
	EINA_PREFERENCES_NO_ERROR = 0,
	EINA_PREFERENCES_CANNOT_ACCESS_PLAYER,
	EINA_PREFERENCES_CANNOT_ACCESS_UI_MANAGER,
	EINA_PREFERENCES_CANNOT_REGISTER_SHARED
};

static gboolean
preferences_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences       *self;

	self = g_new0(EinaPreferences, 1);
	if (!eina_obj_init((EinaObj*) self, plugin, "preferences", EINA_OBJ_NONE, error))
	{
		g_free(self);
		return FALSE;
	}
	plugin->data = self;

	g_signal_connect(gel_app_get_settings(app), "change", (GCallback) change_cb, self);

	return TRUE;
}

static gboolean preferences_fini
(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences *self = EINA_PREFERENCES(plugin->data);

	if (self->n_tabs > 0)
	{
		preferences_deattach_menu(self);
		self->n_tabs = 0;
	}

	if (self->tabs)
	{
		preferences_remove_all_tabs(self);
		g_list_foreach(self->tabs, (GFunc) g_object_unref, NULL);
		g_list_free(self->tabs);
		self->tabs = NULL;
	}

	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

void
eina_preferences_add_tab(EinaPreferences *self, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));
	g_return_if_fail(g_list_find(self->tabs, tab) == NULL);

	self->tabs = g_list_prepend(self->tabs, g_object_ref_sink(tab));
	if (self->dialog)
		eina_preferences_dialog_add_tab(self->dialog, tab);
	self->n_tabs++;

	if (self->n_tabs == 1)
		preferences_attach_menu(self);

	EinaConf *conf = eina_obj_get_settings(self);

	GList *watching = eina_preferences_tab_get_watched(tab);
	GList *iter = watching;
	while (iter)
	{
		GValue *cv = eina_conf_get(conf, (gchar *) iter->data);
		if (cv)
			eina_preferences_tab_set_widget_value(tab, (gchar *) iter->data, cv);
		else
			gel_warn("EinaConf has no value for '%s'", (gchar *) iter->data);
		iter = iter->next;
	}
	g_list_free(watching);
}

void
eina_preferences_remove_tab(EinaPreferences *self, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));
	g_return_if_fail(g_list_find(self->tabs, tab) != NULL);

	g_object_unref(tab);
	if (self->dialog)
		eina_preferences_dialog_remove_tab(self->dialog, tab);
	self->tabs = g_list_remove(self->tabs, tab);
	g_object_unref(G_OBJECT(tab));
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
	g_signal_connect(self->dialog, "value-changed",(GCallback) value_changed_cb, self);

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
		{ "Preferences", GTK_STOCK_PREFERENCES, N_("Preferences"),
		"<Control>p", NULL, G_CALLBACK(menu_activate_cb) },
	};

	const gchar *ui_xml = 
		"<ui>"
		"<menubar name=\"Main\">"
		"<menu name=\"Edit\" action=\"EditMenu\">"
		"  <menuitem name=\"Preferences\" action=\"Preferences\" />"
		"</menu>"
		"</menubar>"
		"</ui>";

	GtkUIManager *ui_manager = eina_window_get_ui_manager(EINA_OBJ_GET_WINDOW(self));
	if (ui_manager == NULL)
	{
		gel_error(N_("Cannot get GtkUIManager for main menu, unable to attach preferences menu"));
		return;
	}

	GError *err = NULL;
	if ((self->ui_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager, ui_xml, -1, &err)) == 0)
	{
		gel_error(N_("Cannot attach menu: %s"), err->message);
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
	GtkUIManager *ui_manager = eina_window_get_ui_manager(EINA_OBJ_GET_WINDOW(self));
	if (ui_manager == NULL)
	{
		gel_error(N_("Cannot get GtkUIManager for main menu, unable to deattach preferences menu"));
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
		build_dialog(self);
		gtk_dialog_run(GTK_DIALOG(self->dialog));
	}
}

static void 
preferences_remove_all_tabs(EinaPreferences *self)
{
	g_return_if_fail(self->dialog != NULL);
	GList *iter = self->tabs;
	while (iter)
	{
		eina_preferences_dialog_remove_tab(self->dialog, EINA_PREFERENCES_TAB(iter->data));
		iter = iter->next;
	}
}

static void
response_cb(GtkWidget *w, gint response, EinaPreferences *self)
{
	if (self->dialog)
	{
		preferences_remove_all_tabs(self);
		gtk_widget_destroy(w);
		self->dialog = NULL;
	}
}

static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferences *self)
{
	if (self->dialog)
	{
		preferences_remove_all_tabs(self);
		self->dialog = NULL;
	}
	return FALSE;
}

static void
value_changed_cb(EinaPreferencesDialog *w, gchar *key, GValue *value, EinaPreferences *self)
{
	EinaConf *conf = eina_obj_get_settings(self);
	eina_conf_set(conf, key, value);
}

static void
change_cb(EinaConf *conf, gchar *key, EinaPreferences *self)
{
	GList *iter = self->tabs;
	while (iter)
	{
		EinaPreferencesTab *tab = EINA_PREFERENCES_TAB(iter->data);
		GtkWidget *w = eina_preferences_tab_get_widget(tab, key);
		if (w)
		{
			eina_preferences_tab_set_widget_value(tab, key, eina_conf_get(eina_obj_get_settings(self), key));
			return;
		}
		iter = iter->next;
	}
	gel_warn(N_("Widget for key '%s' not found"), key);
}


EINA_PLUGIN_SPEC(preferences,
	NULL,
	"settings,window",
	NULL,
	NULL,
	N_("Build-in preferences plugin"),
	NULL,
	NULL,
	preferences_init,
	preferences_fini
);

