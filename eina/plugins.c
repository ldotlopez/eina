/*
 * eina/plugins.c
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
} EinaPlugins;

static void
action_activated_cb(GtkAction *action, EinaPlugins *self);

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

	if (self->widget)
	{
		gtk_widget_destroy((GtkWidget *) self->widget);
		self->widget = NULL;
	}

	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

static void
action_activated_cb(GtkAction *action, EinaPlugins *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "plugins-action"))
	{
		self->widget = eina_plugin_dialog_new(eina_obj_get_app(EINA_OBJ(self)));
		gtk_widget_show((GtkWidget *) self->widget);
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
		gtk_widget_destroy(w);
		break;
	}
}

EINA_PLUGIN_SPEC(plugins,
	NULL,
	"window",

	NULL,
	NULL,

	N_("Build-in plugin manager plugin"),
	NULL,
	NULL,

	plugins_init, plugins_fini
);

