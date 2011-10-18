/*
 * eina/preferences/eina-preferences-plugin.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-preferences-plugin.h"
#include <eina/ext/eina-extension.h>

#define EINA_TYPE_PREFERENCES_PLUGIN         (eina_preferences_plugin_get_type ())
#define EINA_PREFERENCES_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPlugin))
#define EINA_PREFERENCES_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPlugin))
#define EINA_IS_PREFERENCES_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_PREFERENCES_PLUGIN))
#define EINA_IS_PREFERENCES_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_PREFERENCES_PLUGIN))
#define EINA_PREFERENCES_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPluginClass))

typedef struct {
	EinaPreferencesDialog *dialog;
	GList *tabs;

	GtkActionGroup *ag;
	guint ui_merge_id;
} EinaPreferencesPluginPrivate;

EINA_PLUGIN_REGISTER(EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPlugin, eina_preferences_plugin)

GEL_DEFINE_WEAK_REF_CALLBACK(preferences)

void preferences_plugin_add_tab   (EinaPreferencesPlugin *plugin, EinaPreferencesTab *tab);
void preferences_plugin_remove_tab(EinaPreferencesPlugin *plugin, EinaPreferencesTab *tab);

EinaPreferencesPlugin * application_get_preferences(EinaApplication *application);

static void preferences_plugin_attach_menu      (EinaPreferencesPlugin *plugin);
static void preferences_plugin_detach    (EinaPreferencesPlugin *plugin);
static void preferences_plugin_detach_all_tabs(EinaPreferencesPlugin *plugin);
static void menu_activate_cb(GtkAction *action, EinaPreferencesPlugin *plugin);

// Dialog
static void     response_cb    (GtkWidget *w, gint response, EinaPreferencesPlugin *plugin);
static gboolean delete_event_cb(GtkWidget *w, GdkEvent *ev,  EinaPreferencesPlugin *plugin);

enum {
	EINA_PREFERENCES_NO_ERROR = 0,
	EINA_PREFERENCES_CANNOT_ACCESS_PLAYER,
	EINA_PREFERENCES_CANNOT_ACCESS_UI_MANAGER,
	EINA_PREFERENCES_CANNOT_REGISTER_SHARED
};

static EinaActivatable *_fixme_activatable = NULL;

G_MODULE_EXPORT gboolean
eina_preferences_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	_fixme_activatable = activatable;
	return TRUE;
}

G_MODULE_EXPORT gboolean
eina_preferences_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaPreferencesPlugin        *plugin = EINA_PREFERENCES_PLUGIN(activatable);
	EinaPreferencesPluginPrivate *priv   = plugin->priv;

	if (priv->dialog)
	{
		preferences_plugin_detach_all_tabs(plugin);
		gtk_widget_destroy((GtkWidget *) priv->dialog);
		priv->dialog = NULL;
	}

	if (priv->tabs)
	{
		GList *iter = priv->tabs;
		while (iter)
		{
			eina_application_remove_preferences_tab(app, EINA_PREFERENCES_TAB(iter->data));
			iter = priv->tabs;
		}
		g_list_free(priv->tabs);
		priv->tabs = NULL;
		// Edit/Preferences menu is deattached automatically after remove last tab
	}

	_fixme_activatable = NULL;

	return TRUE;
}

EinaPreferencesPlugin *
application_get_preferences(EinaApplication *application)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(application), NULL);
	return EINA_PREFERENCES_PLUGIN(_fixme_activatable);
}

void
eina_application_add_preferences_tab(EinaApplication *application, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_APPLICATION(application));

	EinaPreferencesPlugin *plugin = application_get_preferences(application);
	g_return_if_fail(plugin != NULL);

	preferences_plugin_add_tab(plugin, tab);
}

void
eina_application_remove_preferences_tab(EinaApplication *application, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_APPLICATION(application));

	EinaPreferencesPlugin *plugin = application_get_preferences(application);
	g_return_if_fail(plugin != NULL);

	preferences_plugin_remove_tab(plugin, tab);
}

void
preferences_plugin_add_tab(EinaPreferencesPlugin *plugin, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));
	g_return_if_fail(g_list_find(priv->tabs, tab) == NULL);

	priv->tabs = g_list_prepend(priv->tabs, g_object_ref(tab)); // owning
	g_object_weak_ref((GObject *) tab, (GWeakNotify) preferences_weak_ref_cb, NULL);

	if (priv->dialog)
		eina_preferences_dialog_add_tab(priv->dialog, tab);

	// Just one tab
	if (priv->tabs && !priv->tabs->next)
		preferences_plugin_attach_menu(plugin);
}

void
preferences_plugin_remove_tab(EinaPreferencesPlugin *plugin, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));
	g_return_if_fail(g_list_find(priv->tabs, tab) != NULL);

	if (priv->dialog)
		eina_preferences_dialog_remove_tab(priv->dialog, tab);

	g_object_weak_unref((GObject *) tab, (GWeakNotify) preferences_weak_ref_cb, NULL);
	priv->tabs = g_list_remove(priv->tabs, tab);

	g_object_unref(G_OBJECT(tab));

	// No tabs
	if (!priv->tabs)
		preferences_plugin_detach(plugin);
}

static void
preferences_plugin_build_dialog(EinaPreferencesPlugin *plugin)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	if (GTK_IS_DIALOG(priv->dialog))
		return;

	EinaApplication *app = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	priv->dialog = eina_preferences_dialog_new((GtkWindow *) eina_application_get_window(app));
	g_object_set((GObject*) priv->dialog,
		"title", N_("Preferences"),
		"window-position", GTK_WIN_POS_CENTER_ON_PARENT,
		"width-request",  600,
		"height-request", 400,
		NULL);
	g_signal_connect(priv->dialog, "response",     (GCallback) response_cb,     plugin);
	g_signal_connect(priv->dialog, "delete-event", (GCallback) delete_event_cb, plugin);

	GList *iter = g_list_last(priv->tabs);
	while (iter)
	{
		eina_preferences_dialog_add_tab(priv->dialog, EINA_PREFERENCES_TAB(iter->data));
		iter = iter->prev;
	}
}

static void
preferences_plugin_attach_menu(EinaPreferencesPlugin *plugin)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

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

	EinaApplication *app        = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	GtkUIManager    *ui_manager = eina_application_get_window_ui_manager(app);

	if (ui_manager == NULL)
	{
		g_warning(N_("Cannot get GtkUIManager for main menu, unable to attach preferences menu"));
		return;
	}

	GError *err = NULL;
	if ((priv->ui_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager, ui_xml, -1, &err)) == 0)
	{
		g_warning(N_("Cannot attach menu: %s"), err->message);
		g_error_free(err);
	}

	priv->ag = gtk_action_group_new("preferences");
	gtk_action_group_add_actions(priv->ag, action_entries, G_N_ELEMENTS(action_entries), plugin);
	gtk_ui_manager_insert_action_group(ui_manager, priv->ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);
}

static void
preferences_plugin_detach(EinaPreferencesPlugin *plugin)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	EinaApplication *app        = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	GtkUIManager    *ui_manager = eina_application_get_window_ui_manager(app);
	if (ui_manager == NULL)
	{
		g_warning(N_("Cannot get GtkUIManager for main menu, unable to deattach preferences menu"));
		return;
	}

	g_warn_if_fail(priv->ui_merge_id > 0);
	if (priv->ui_merge_id)
	{
		gtk_ui_manager_remove_ui(ui_manager, priv->ui_merge_id);
		priv->ui_merge_id = 0;
	}

	g_warn_if_fail(priv->ag != NULL);
	if (priv->ag)
	{
		gtk_ui_manager_remove_action_group(ui_manager, priv->ag);
		g_object_unref(priv->ag);
		priv->ag = NULL;
	}
}

static void
menu_activate_cb(GtkAction *action, EinaPreferencesPlugin *plugin)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	if (g_str_equal(gtk_action_get_name(action), "preferences"))
	{
		preferences_plugin_build_dialog(plugin);
		gtk_dialog_run(GTK_DIALOG(priv->dialog));
	}
}

static void 
preferences_plugin_detach_all_tabs(EinaPreferencesPlugin *plugin)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	g_warn_if_fail(priv->dialog);
	if (priv->dialog)
	{
		GList *iter = priv->tabs;
		while (iter)
		{
			eina_preferences_dialog_remove_tab(priv->dialog, EINA_PREFERENCES_TAB(iter->data));
			iter = iter->next;
		}
	}
}

static void
response_cb(GtkWidget *w, gint response, EinaPreferencesPlugin *plugin)
{
	g_return_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin));
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	preferences_plugin_detach_all_tabs(plugin);
	priv->dialog = NULL;

	gtk_widget_destroy(w);
}

static gboolean
delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPreferencesPlugin *plugin)
{
	g_return_val_if_fail(EINA_IS_PREFERENCES_PLUGIN(plugin), FALSE);
	EinaPreferencesPluginPrivate *priv = plugin->priv;

	preferences_plugin_detach_all_tabs(plugin);
	priv->dialog = NULL;

	return FALSE;
}

