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

#define EINA_PLUGINS_DOMAIN       EINA_DOMAIN".preferences.plugins"
#define EINA_PLUGINS_PLUGINS_KEY "plugins"

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

static void
settings_changed_cb(GSettings *settings, gchar *key, EinaPlugins *self);

G_MODULE_EXPORT gboolean
plugins_plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self = g_new0(EinaPlugins, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "plugins", 0, error))
	{
		g_free(self);
		return FALSE;
	}
	gel_plugin_set_data(plugin, self);

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

	GSettings *settings = gel_app_get_gsettings(app, EINA_PLUGINS_DOMAIN);
	gchar **plugins = g_settings_get_strv(settings, EINA_PLUGINS_PLUGINS_KEY);

	for (guint i = 0; plugins && plugins[i]; i++)
	{
		gchar **parts  = g_strsplit(plugins[i], ":", 2);
		GError *error = NULL;
		if (parts[0] && !gel_app_load_plugin_by_pathname(app, parts[0], &error))
		{
			gel_error(N_("Cannot load %s: %s"), plugins[i], error->message);
			g_error_free(error);
		}
		gel_free_and_invalidate(parts, NULL, g_strfreev);
	}
	g_signal_connect(settings, "changed", (GCallback) settings_changed_cb, self);

	return TRUE;
}

G_MODULE_EXPORT gboolean
plugins_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self = EINA_PLUGIN_DATA(plugin);
	
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

	GList *plugins = gel_app_get_plugins(app);
	gchar **strv = g_new0(gchar *, g_list_length(plugins) + 1);
	gint i = 0;

	GList *l = plugins;
	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);
		if (gel_plugin_get_info(plugin)->pathname)
			strv[i++] = (gchar *) gel_plugin_stringify(plugin);
		l = l->next;
	}
	g_list_free(plugins);

	GSettings *settings = eina_obj_get_gsettings(self, EINA_PLUGINS_DOMAIN);
	g_settings_set_strv(settings, EINA_PLUGINS_PLUGINS_KEY, (const gchar * const*) strv);
	g_free(strv);
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
	// GelPlugin *plugin;
	switch (response)
	{
	case EINA_PLUGIN_DIALOG_RESPONSE_INFO:
		#if 0
		plugin = eina_plugin_dialog_get_selected_plugin(self->widget);
		if (!plugin)
		{
			gel_warn(N_("No plugin selected"));
			return;
		}
		EinaPluginProperties *props = eina_plugin_properties_new(plugin);
		g_signal_connect(props, "response", (GCallback) gtk_widget_destroy, NULL);
		gtk_widget_show((GtkWidget *) props);
		#endif
		break;

	default:
		self->widget = NULL;
		disable_watch(self);
		gtk_widget_destroy(w);
		break;
	}
}

static void
settings_changed_cb(GSettings *settings, gchar *key, EinaPlugins *self)
{
	if (g_str_equal(key, EINA_PLUGINS_PLUGINS_KEY))
	{
		g_warning(N_("TODO: Implement a plugin sync routine"));
	}

	else
	{
		g_warning(N_("Unhanded key '%s'"), key);
	}
}

EINA_PLUGIN_INFO_SPEC(plugins,
	NULL,
	"settings,window",

	NULL,
	NULL,

	N_("Build-in plugin manager plugin"),
	NULL,
	NULL
);

