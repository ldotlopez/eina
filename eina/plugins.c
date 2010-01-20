/*
 * eina/plugins.c
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

#define GEL_DOMAIN "Eina::Plugins"
#define EINA_PLUGIN_DATA_TYPE EinaPlugins

#include <gmodule.h>
#include <config.h>
#include <eina/eina-plugin.h>
#include <eina/ext/eina-plugin-dialog.h>
#include <eina/ext/eina-plugin-properties.h>

typedef struct {
	EinaObj         parent;

	// UI Mananger
	GtkActionGroup *action_group;
	guint           ui_mng_merge;

	EinaPluginDialog *widget;
	gboolean watching;
} EinaPlugins;

static void
enable_watch(EinaPlugins *self);
static void
disable_watch(EinaPlugins *self);
static void
update_plugins_list(EinaPlugins *self);
static void
action_activated_cb(GtkAction *action, EinaPlugins *self);
static void
response_cb(GtkWidget *w, gint response, EinaPlugins *self);

static gchar *ui_mng_xml =
"<ui>"
"  <menubar name='Main'>"
"    <menu name='Edit' action='EditMenu'>"
"      <menuitem name='Plugins' action='plugins-action' />"
"    </menu>"
"  </menubar>"
"</ui>";

static GtkActionEntry ui_mng_action_entries[] = {
	{ "plugins-action", EINA_STOCK_PLUGIN, N_("Plugins"),
	"<Control>u", N_("Select plugins"), (GCallback) action_activated_cb }
};

static void
response_cb(GtkWidget *w, gint response, EinaPlugins *self);

static gboolean
plugins_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self = g_new0(EinaPlugins, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "plugins", 0, error))
	{
		g_free(self);
		return FALSE;
	}
	plugin->data = self;

	// Add on menu
	self->action_group = gtk_action_group_new("plugins");
	gtk_action_group_add_actions_full(self->action_group, ui_mng_action_entries, G_N_ELEMENTS(ui_mng_action_entries), 
		self, NULL);

	GError *err = NULL;
	GtkUIManager *ui_mng = eina_window_get_ui_manager(gel_app_get_window(app));
	gtk_ui_manager_insert_action_group(ui_mng, self->action_group, G_MAXINT);
	if ((self->ui_mng_merge = gtk_ui_manager_add_ui_from_string(ui_mng, ui_mng_xml, -1, &err)) == 0)
	{
		gel_warn(N_("Cannot merge menu: %s"), err->message);
		g_error_free(err);
	}
	else
	{
		gtk_ui_manager_ensure_update(ui_mng);
	}

	// Now load all plugins
	self->watching = FALSE;
	EinaConf *conf = eina_obj_get_settings(self);
	eina_conf_delete_key(conf, "/plugins/enabled");
	const gchar *tmp = eina_conf_get_str(conf, "/core/plugins", NULL);
	if (!tmp)
		return TRUE;

	gint i = 0;
	gchar **plugins = g_strsplit(tmp, ",", 0);
	for (i = 0; plugins[i] && plugins[i][0]; i++)
	{
		GError *error = NULL;
		if (!gel_app_load_plugin_by_pathname(app, plugins[i], &error))
		{
			gel_error(N_("Cannot load %s: %s"), plugins[i], error->message);
			g_error_free(error);
		}
	}

	return TRUE;
}

static gboolean
plugins_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self = (EinaPlugins *) plugin->data;
	
	GtkUIManager *ui_mng = eina_window_get_ui_manager(eina_obj_get_window(EINA_OBJ(self)));
	gtk_ui_manager_remove_ui(ui_mng, self->ui_mng_merge);
	gtk_ui_manager_remove_action_group(ui_mng, self->action_group);
	g_object_unref(self->action_group);

	disable_watch(self);

	if (self->widget)
	{
		gtk_widget_destroy((GtkWidget *) self->widget);
		self->widget = NULL;
	}

	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

static void
enable_watch(EinaPlugins *self)
{
	if (!self->watching)
	{
		self->watching = TRUE;
		g_signal_connect_swapped(eina_obj_get_app(self), "plugin-init", (GCallback) update_plugins_list, self);
		g_signal_connect_swapped(eina_obj_get_app(self), "plugin-fini", (GCallback) update_plugins_list, self);
	}
}

static void
disable_watch(EinaPlugins *self)
{
	if (self->watching)
	{
		self->watching = FALSE;
		g_signal_handlers_disconnect_by_func(eina_obj_get_app(self), update_plugins_list, self);
		g_signal_handlers_disconnect_by_func(eina_obj_get_app(self), update_plugins_list, self);
	}
}

static void
update_plugins_list(EinaPlugins *self)
{
	GelApp *app = eina_obj_get_app(self);
	GList *list = NULL;

	GList *plugins = gel_app_get_plugins(app);
	GList *l = plugins;
	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);
		const gchar *pathname = gel_plugin_get_pathname(plugin);
		if (pathname && gel_plugin_is_enabled(plugin))
			list = g_list_prepend(list, g_strdup(pathname));
		l = l->next;
	}
	g_list_free(plugins);

	gchar *list_str = gel_list_join(",", list);
	gel_list_deep_free(list, g_free);
	eina_conf_set_str(eina_obj_get_settings(self), "/core/plugins", list_str);
	g_free(list_str);

}

static void
action_activated_cb(GtkAction *action, EinaPlugins *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "plugins-action"))
	{
		self->widget = eina_plugin_dialog_new(eina_obj_get_app(EINA_OBJ(self)));
		gtk_widget_show((GtkWidget *) self->widget);
		enable_watch(self);
		g_signal_connect(self->widget, "response", (GCallback) response_cb, self);
	}
	else
		gel_warn(N_("Unknow action: %s"), name);
}

static void
response_cb(GtkWidget *w, gint response, EinaPlugins *self)
{
	GelPlugin *plugin;
	switch (response)
	{
	case EINA_PLUGIN_DIALOG_RESPONSE_INFO:
		plugin = eina_plugin_dialog_get_selected_plugin(self->widget);
		if (!plugin)
		{
			gel_warn(N_("No plugin selected"));
			return;
		}
		EinaPluginProperties *props = eina_plugin_properties_new(plugin);
		g_signal_connect(props, "response", (GCallback) gtk_widget_destroy, NULL);
		gtk_widget_show((GtkWidget *) props);
		break;

	default:
		self->widget = NULL;
		disable_watch(self);
		gtk_widget_destroy(w);
		break;
	}
}

EINA_PLUGIN_SPEC(plugins,
	NULL,
	"settings,window",

	NULL,
	NULL,

	N_("Build-in plugin manager plugin"),
	NULL,
	NULL,

	plugins_init, plugins_fini
);

