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
	guint n_tabs;

	GtkActionGroup *ag;
	guint ui_merge_id;

	GHashTable *entries;
};

static void
attach_menu(EinaPreferences *self);
static void
deattach_menu(EinaPreferences *self);
static void
menu_activate_cb(GtkAction *action, EinaPreferences *self);

// Dialog
static void
response_cb(GtkWidget *w, gint response, EinaPreferences *self);
static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferences *self);
static void
value_changed_cb(EinaPreferencesDialog *w, gchar *group, gchar *object, GValue *value, EinaPreferences *self);

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

	self->entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) eina_preferences_dialog_entry_free);
	
	g_signal_connect(gel_app_get_settings(app), "change", (GCallback) change_cb, self);

	return TRUE;
}

static gboolean preferences_fini
(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPreferences *self = EINA_PREFERENCES(plugin->data);

	if (self->n_tabs > 0)
		deattach_menu(self);
	
	if (self->dialog)
		gtk_widget_destroy((GtkWidget *) self->dialog);
	g_hash_table_destroy(self->entries);
	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

void
eina_preferences_add_tab_full(EinaPreferences *self, gchar *group, gchar *xml, gchar *root, gchar **objects, guint n,
    GtkImage *icon, GtkLabel *label)
{
	EinaPreferencesDialogEntry *entry = eina_preferences_dialog_entry_new(group, xml, root, objects, n, icon, label);
	g_hash_table_insert(self->entries, g_strdup(group), entry);
	self->n_tabs++;

	if (self->n_tabs == 1)
		attach_menu(self);
}

void
eina_preferences_remove_tab(EinaPreferences *self, gchar *group)
{
	g_hash_table_remove(self->entries, group);
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

	EinaConf *conf = eina_obj_get_settings(self);

	GList *keys = g_list_reverse(g_list_sort(g_hash_table_get_keys(self->entries), (GCompareFunc) eina_preferecens_dialog_entry_cmp));
	GList *iter = keys;
	while (iter)
	{
		gchar *key = (gchar *) iter->data;
		EinaPreferencesDialogEntry *value = (EinaPreferencesDialogEntry *) g_hash_table_lookup(self->entries, key);
		if (value == NULL)
		{
			gel_warn(N_("Found NULL key"));
			iter = iter->next;
			continue;
		}

		eina_preferences_dialog_add_tab_from_entry(self->dialog, value);
		gint i;
		for (i = 0; i < value->n; i++)
		{
			GValue *v = eina_conf_get(conf, value->objects[i]);
			if (v != NULL)
				eina_preferences_dialog_set_value(self->dialog, value->group, value->objects[i], v);
		}

		iter = iter->next;
	}
	g_list_free(keys);
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
deattach_menu(EinaPreferences *self)
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
response_cb(GtkWidget *w, gint response, EinaPreferences *self)
{
	gtk_widget_destroy(w);
	self->dialog = NULL;
}

static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferences *self)
{
	self->dialog = NULL;
	return FALSE;
}

static void
value_changed_cb(EinaPreferencesDialog *w, gchar *group, gchar *object, GValue *value, EinaPreferences *self)
{
	EinaConf *conf = eina_obj_get_settings(self);
	// gchar *path = g_strdup_printf("/%s/%s", group, object);

	GType type = G_VALUE_TYPE(value);
	if (type == G_TYPE_BOOLEAN)
		eina_conf_set_bool(conf, object, g_value_get_boolean(value));

	else if (type == G_TYPE_INT)
		eina_conf_set_int(conf, object, g_value_get_int(value));

	else if (type == G_TYPE_STRING)
		eina_conf_set_string(conf, object, (gchar *) g_value_get_string(value));

	else
		gel_warn(N_("Type %s for object %s is not handled"), g_type_name(type), object);

	// g_free(path);
}

static void
change_cb(EinaConf *conf, gchar *key, EinaPreferences *self)
{
	if (self->dialog == NULL)
		return;

	/*
	gchar **comp = g_strsplit(key, "/", 0);
	if (g_strv_length(comp) != 2)
		return;
	*/
	GValue *value = eina_conf_get(conf, key);
	eina_preferences_dialog_set_value(self->dialog, NULL, key, value);
	// g_strfreev(comp);
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

