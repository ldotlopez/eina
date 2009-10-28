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
#include <eina/ext/eina-plugin-properties.h>

typedef struct {
	EinaObj         parent;

	// UI Mananger
	GtkActionGroup *action_group;
	guint           ui_mng_merge;

	// Plugin tracking
	GList          *plugins;

	GtkWidget      *window;
} EinaPlugins;

enum {
	PLUGINS_RESPONSE_PROPERTIES = 1
};

enum {
	PLUGINS_COLUMN_ENABLED,
	PLUGINS_COLUMN_ICON,
	PLUGINS_COLUMN_MARKUP,
	PLUGINS_COLUMN_PLUGIN
};

static gboolean
plugins_cleanup_dialog(EinaPlugins *self);
static gint
gel_plugin_cmp_by_usage(GelPlugin *a, GelPlugin *b);
static gint
gel_plugin_cmp_by_name(GelPlugin *a, GelPlugin *b);
static void
app_init_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self);
static void
app_fini_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self);
static void
action_activated_cb(GtkAction *action, EinaPlugins *self);
static void
window_response_cb(GtkWidget *w, gint response, EinaPlugins *self);
static void
enabled_renderer_toggled_cb(GtkCellRendererToggle *render, gchar *path, EinaPlugins *self);

static gchar *ui_mng_xml =
"<ui>"
"  <menubar name='Main'>"
"    <menu name='Edit' action='EditMenu'>"
"      <menuitem name='Plugins' action='plugins-action' />"
"    </menu>"
"  </menubar>"
"</ui>";
static GtkActionEntry ui_mng_action_entries[] = {
	{ "plugins-action", NULL , N_("Plugins"),
	"<Control>u", N_("Select plugins"), (GCallback) action_activated_cb }
};

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

	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

static void
plugins_show_dialog(EinaPlugins *self)
{
	GError *err = NULL;
	if (!eina_obj_load_default_ui(EINA_OBJ(self), &err))
	{
		gel_warn(N_("Cannot load default ui file: %s"), err->message);
		g_error_free(err);
		return;
	}

	// Connect signals
	g_signal_connect(eina_obj_get_object(self, "enabled-renderer"), "toggled", (GCallback) enabled_renderer_toggled_cb, self);
	g_signal_connect(eina_obj_get_app(EINA_OBJ(self)), "plugin-init", (GCallback) app_init_plugin_cb, self);
	g_signal_connect(eina_obj_get_app(EINA_OBJ(self)), "plugin-fini", (GCallback) app_fini_plugin_cb, self);

	GtkTreeView *tv = eina_obj_get_typed(self, GTK_TREE_VIEW, "treeview");
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
	gtk_list_store_clear(model);

	// Get plugins
	self->plugins = gel_app_query_plugins(eina_obj_get_app(self));
	GList *internal_plugins = NULL;
	GList *external_plugins = NULL;
	GList *l = self->plugins;
	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);
		if (gel_plugin_get_pathname(plugin))
			external_plugins = g_list_prepend(external_plugins, plugin);
		else
			internal_plugins = g_list_prepend(internal_plugins, plugin);
		l = l->next;
	}
	g_list_foreach(self->plugins, (GFunc) gel_plugin_add_lock, NULL);

	// Tabs
	GtkNotebook *tabs = eina_obj_get_typed(self, GTK_NOTEBOOK, "tabs");
	gtk_notebook_set_show_tabs(tabs, FALSE);
	if (external_plugins == NULL)
	{
		gtk_notebook_set_current_page(tabs, 0);
		g_list_free(internal_plugins);
		return;
	}
	gtk_notebook_set_current_page(tabs, 1);
	internal_plugins = g_list_sort(internal_plugins, (GCompareFunc) gel_plugin_cmp_by_name);
	external_plugins = g_list_sort(external_plugins, (GCompareFunc) gel_plugin_cmp_by_name);

	// Fill notebook
	l = external_plugins;
	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);

		GdkPixbuf *pb = NULL;
		gchar *pb_path = NULL;

		if (plugin->icon)
		{
			pb_path = gel_plugin_get_resource(plugin, GEL_RESOURCE_IMAGE, (gchar *) plugin->icon);
			if (pb_path)
			{
				pb = gdk_pixbuf_new_from_file_at_size(pb_path, 64, 64, NULL);
				g_free(pb_path);
			}
		}

		if (pb == NULL)
		{
			pb_path = gel_plugin_get_resource(eina_obj_get_plugin(EINA_OBJ(self)), GEL_RESOURCE_IMAGE, "plugin.png");
			pb = gdk_pixbuf_new_from_file_at_size(pb_path, 64, 64, NULL);
		}

		gchar *markup = g_strdup_printf("<span size='x-large' weight='bold'>%s</span>\n%s",
			plugin->name,
			plugin->short_desc
			);

		GtkTreeIter iter;
		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter,
			PLUGINS_COLUMN_ENABLED, gel_plugin_is_enabled(plugin),
			PLUGINS_COLUMN_ICON,    pb,
			PLUGINS_COLUMN_MARKUP,  markup,
			PLUGINS_COLUMN_PLUGIN,  plugin,
			-1);
		l = l->next;
	}
	g_list_free(internal_plugins);
	g_list_free(external_plugins);

	self->window = eina_obj_get_widget(self, "main-window");
	gtk_widget_realize(self->window);

	GdkScreen *screen = gtk_window_get_screen((GtkWindow *) self->window);
	gint w = gdk_screen_get_width(screen)  / 4;
	gint h = gdk_screen_get_height(screen) / 2;
	gtk_window_resize((GtkWindow *) self->window, w, h);

	g_signal_connect(self->window, "response", (GCallback) window_response_cb, self);
	g_signal_connect_swapped(self->window, "delete-event", (GCallback) plugins_cleanup_dialog, self);
	gtk_widget_show(self->window);
}

static gboolean
plugins_cleanup_dialog(EinaPlugins *self)
{
	GelApp *app = eina_obj_get_app(EINA_OBJ(self));

	g_signal_handlers_disconnect_by_func(app, app_init_plugin_cb, self);
	g_signal_handlers_disconnect_by_func(app, app_fini_plugin_cb, self);
	g_signal_handlers_disconnect_by_func(eina_obj_get_object(self, "enabled-renderer"), enabled_renderer_toggled_cb, self);

	// Remove locks on plugins
	g_list_foreach(self->plugins, (GFunc) gel_plugin_remove_lock, NULL);

	// Sort plugins by usage
	GList *tmp = g_list_sort(self->plugins, (GCompareFunc) gel_plugin_cmp_by_usage);
	GList *l = tmp;
	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);
		if (!gel_plugin_is_enabled(plugin))
			gel_app_unload_plugin(app, plugin, NULL);
		l = l->next;
	}

	// Purge app
	gel_app_purge(app);

	g_object_unref(EINA_OBJ(self)->ui);
	EINA_OBJ(self)->ui = NULL;

	self->window = NULL;

	return FALSE;
}

static gint
gel_plugin_cmp_by_usage(GelPlugin *a, GelPlugin *b)
{
	return gel_plugin_get_usage(b) - gel_plugin_get_usage(a);
}

static gint
gel_plugin_cmp_by_name(GelPlugin *a, GelPlugin *b)
{
	return strcmp(a->name, b->name);
}

static gboolean
plugins_get_iter_from_plugin(EinaPlugins *self, GtkTreeIter *iter, GelPlugin *plugin)
{
	GtkTreeModel *model = gtk_tree_view_get_model(eina_obj_get_typed(self, GTK_TREE_VIEW, "treeview"));
	if (!gtk_tree_model_get_iter_first(model, iter))
		return FALSE;
	do
	{
		GelPlugin *test;
		gtk_tree_model_get(model, iter, PLUGINS_COLUMN_PLUGIN, &test, -1);
		if (test == plugin)
			return TRUE;

	} while (gtk_tree_model_iter_next(model, iter));

	return FALSE;
}

static void
plugins_update_plugin(EinaPlugins *self, GelPlugin *plugin)
{
	GtkTreeIter iter;
	if (!plugins_get_iter_from_plugin(self, &iter, plugin))
	{
		gel_warn(N_("Loaded unknow plugin: %s"), gel_plugin_stringify(plugin));
		return;
	}

	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(eina_obj_get_typed(self, GTK_TREE_VIEW, "treeview")));
	gtk_list_store_set(model, &iter, PLUGINS_COLUMN_ENABLED, gel_plugin_is_enabled(plugin), -1);
}

static void
app_init_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self)
{
	plugins_update_plugin(self, plugin);
}

static void
app_fini_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self)
{
	plugins_update_plugin(self, plugin);
}

static void
action_activated_cb(GtkAction *action, EinaPlugins *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "plugins-action"))
		plugins_show_dialog(self);
	else
		gel_warn(N_("Unknow action: %s"), name);
}

static void
window_response_cb(GtkWidget *w, gint response, EinaPlugins *self)
{
	GtkTreeSelection *tvsel;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GelPlugin    *plugin;

	switch (response)
	{
	case PLUGINS_RESPONSE_PROPERTIES:
		tvsel = gtk_tree_view_get_selection(eina_obj_get_typed(EINA_OBJ(self), GTK_TREE_VIEW, "treeview"));
		if (!gtk_tree_selection_get_selected(tvsel, &model, &iter))
		{
			gel_warn(N_("Error getting selection info"));
			return;
		}

		gtk_tree_model_get(model, &iter, PLUGINS_COLUMN_PLUGIN, &plugin, -1);

		EinaPluginProperties *pp = eina_plugin_properties_new(plugin);
		g_signal_connect(pp, "response", (GCallback) gtk_widget_destroy, NULL);
		gtk_widget_show((GtkWidget *) pp);

		break;

	default:
		plugins_cleanup_dialog(self);
		gtk_widget_destroy(w);
		break;
	}
}

static void
enabled_renderer_toggled_cb(GtkCellRendererToggle *render, gchar *path, EinaPlugins *self)
{
	GtkTreePath *tp = gtk_tree_path_new_from_string(path);
	GtkTreeIter iter;

	GtkTreeView *tv = eina_obj_get_typed(self, GTK_TREE_VIEW, "treeview");
	GtkTreeModel *model = gtk_tree_view_get_model(tv);
	gtk_tree_model_get_iter(model, &iter, tp);
	gtk_tree_path_free(tp);

	GelPlugin *plugin;
	gtk_tree_model_get(model, &iter,
		PLUGINS_COLUMN_PLUGIN, &plugin,
		-1);

	if (gel_plugin_is_enabled(plugin))
	{
		gel_app_unload_plugin(eina_obj_get_app(self), plugin, NULL);
	}
	else
	{
		gel_warn("Enable");
		gel_app_load_plugin(eina_obj_get_app(self), (gchar *) gel_plugin_get_pathname(plugin), (gchar *) plugin->name, NULL);
	}
}

EINA_PLUGIN_SPEC(plugins,
	NULL,
	NULL,

	NULL,
	NULL,

	N_("Build-in plugin manager plugin"),
	NULL,
	NULL,

	plugins_init, plugins_fini
);

