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
#include <eina/window.h>
#include <eina/ext/eina-plugin-properties.h>

#if 1
#define debug(...) gel_warn(__VA_ARGS__)
#else
#define debug(...)
#endif

typedef struct {
	EinaObj         parent;

	// UI Mananger
	GtkActionGroup *action_group;
	guint           ui_mng_merge;

	// Plugin tracking
	GList          *plugins;

	GtkWidget      *window;
#if 0
	EinaConf       *conf;
	GtkWidget      *window;
	GtkTreeView    *treeview;
	GtkListStore   *model;
	
	GList          *visible_plugins;
	GelPlugin      *active_plugin;

	GtkActionEntry *ui_actions;
	GtkActionGroup *ui_mng_ag;
	guint           ui_mng_merge_id;
#endif
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

#if 0
typedef enum {
	EINA_PLUGINS_NO_ERROR,
	EINA_PLUGINS_ERROR_NO_SETTINGS,
	EINA_PLUGINS_ERROR_NO_PLAYER,
	EINA_PLUGINS_ERROR_MERGE_MENU
} EinaPluginsError;

enum {
	PLUGINS_TAB_NO_PLUGINS = 0,
	PLUGINS_TAB_PLUGINS
};

enum {
	RESPONSE_CLOSE = GTK_RESPONSE_CLOSE,
	RESPONSE_INFO  = 1
};

enum {
	PLUGINS_COLUMN_ENABLED,
	PLUGINS_COLUMN_ICON,
	PLUGINS_COLUMN_MARKUP,

	PLUGINS_N_COLUMNS
};

static void
app_init_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self);
static void
app_fini_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self);

static void
plugins_show(EinaPlugins *self);
static void
plugins_hide(EinaPlugins *self);

static void
plugins_load_plugins(EinaPlugins *self);
static void
plugins_unload_plugins(EinaPlugins *self, gboolean all);
static void
plugins_build_treeview(EinaPlugins *self);

static void
plugins_window_response_cb (GtkDialog *dialog, gint response_id, EinaPlugins *self);   
static void
plugins_treeview_cursor_changed_cb(GtkWidget *w, EinaPlugins *self);
static void
plugins_cell_renderer_toggle_toggled_cb(GtkCellRendererToggle *w, gchar *path, EinaPlugins *self);
static void
plugins_menu_activate_cb(GtkAction *action, EinaPlugins *self);

static GQuark
plugins_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-plugins");
	return ret;
}

//--
// Init/Exit functions 
//--
static gboolean
plugins_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self =  g_new0(EinaPlugins, 1);

	if (!eina_obj_init(EINA_OBJ(self), plugin, "plugins", EINA_OBJ_GTK_UI, error))
	{
		g_free(self);
		return FALSE;
	}

	// Load settings
	if ((self->conf = GEL_APP_GET_SETTINGS(app)) == NULL)
	{
		g_set_error(error, plugins_quark(), EINA_PLUGINS_ERROR_NO_SETTINGS,
			N_("Cannot get settings object"));
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	// Setup signals
	self->window   = eina_obj_get_widget(self, "main-window");
	self->treeview = eina_obj_get_typed(self, GTK_TREE_VIEW, "treeview");
	self->model    = eina_obj_get_typed(self, GTK_LIST_STORE, "liststore");

	plugin->data = self;

	// Setup UI

	// Set size of dialog to more or less safe values w/4 x h/2, relative to
	// the size of the screen
	gtk_widget_realize(self->window);
	GdkScreen *screen = gtk_window_get_screen((GtkWindow *) self->window);
	gint w = gdk_screen_get_width(screen)  / 4;
	gint h = gdk_screen_get_height(screen) / 2;
	gtk_window_resize((GtkWindow *) self->window, w, h);

	GtkUIManager *ui_manager = eina_window_get_ui_manager(EINA_OBJ_GET_WINDOW(self));
	self->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager,
		"<ui>"
		"<menubar name=\"Main\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Plugins\" action=\"Plugins\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, error);
	if (self->ui_mng_merge_id == 0)
	{
		g_set_error(error, plugins_quark(), EINA_PLUGINS_ERROR_MERGE_MENU,
			N_("Cannot merge menu"));
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	GtkActionEntry action_entries[] = {
		{ "Plugins", GTK_STOCK_EXECUTE, N_("Plugins"),
	    "<control>u", NULL, G_CALLBACK(plugins_menu_activate_cb) }
	};
	
	self->ui_mng_ag = gtk_action_group_new("plugins");
	gtk_action_group_add_actions(self->ui_mng_ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(ui_manager, self->ui_mng_ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);

	// Setup signals
	g_signal_connect_swapped(self->window, "delete-event", (GCallback) plugins_hide, self);
	g_signal_connect_swapped(self->window, "response",     (GCallback) plugins_hide, self);

	g_signal_connect(self->window,   "response",       (GCallback) plugins_window_response_cb, self);
	g_signal_connect(self->treeview, "cursor-changed", (GCallback) plugins_treeview_cursor_changed_cb, self);
	g_signal_connect(eina_obj_get_object(self, "toggle-renderer"), "toggled",
		(GCallback) plugins_cell_renderer_toggle_toggled_cb, self);


	// Load plugins
	const gchar *plugins_str = eina_conf_get_str(self->conf, "/plugins/enabled", NULL);
	if (plugins_str)
	{
		gchar **plugins = g_strsplit(plugins_str, ",", 0);
		gint i;
		for (i = 0; plugins[i] != NULL; i++)
		{
			GError *err = NULL;

			GelPlugin *plugin = gel_app_load_plugin_by_pathname(app, plugins[i], &err);
			if (plugin)
				continue;

			gchar *dirname = g_path_get_dirname(plugins[i]);
			gchar *plugin_name = g_path_get_basename(dirname);
			g_free(dirname);
			gel_warn("Plugin %s loaded by pathname failed: %s.\nTrying with name %s.", plugins[i], err->message, plugin_name);
			g_clear_error(&err);

			plugin = gel_app_load_plugin_by_name(app, plugin_name, &err);
			if (plugin)
			{
				g_free(plugin_name);
				continue;
			}

			gel_error("Cannot load plugin (neither path or name)'%s': %s", plugin_name, err->message);
			g_free(plugin_name);
			gel_free_and_invalidate(err, NULL, g_error_free);
		}
		g_strfreev(plugins);
	}
	plugin->data = self;

	return TRUE;
}

static gboolean
plugins_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self = EINA_PLUGIN_DATA(plugin);

	// Unload plugins
	plugins_unload_plugins(self, TRUE);
	
	// Unmerge menu
	GtkUIManager *ui_mng;
	if ((ui_mng = eina_window_get_ui_manager(EINA_OBJ_GET_WINDOW(self))) != NULL)
	{
		gtk_ui_manager_remove_action_group(ui_mng,
			self->ui_mng_ag);
		gtk_ui_manager_remove_ui(ui_mng,
			self->ui_mng_merge_id);
		g_object_unref(self->ui_mng_ag);
	}
	gtk_widget_destroy(self->window);

	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

//--
// Private functions
//--
static void
plugins_load_plugins(EinaPlugins *self)
{
	GList *plugins, *iter;
	iter = plugins = gel_app_query_plugins(eina_obj_get_app(self));
	// gel_warn("App is aware of %d plugins", g_list_length(plugins));
	while (iter)
	{
		GelPlugin *plugin = (GelPlugin *) iter->data;
		if (gel_plugin_get_pathname(plugin) == NULL)
		{
			iter = iter->next;
			continue;
		}

		// Add a lock on visible plugins
		self->visible_plugins = g_list_prepend(self->visible_plugins, plugin);
		gel_plugin_add_lock(plugin);

		// gel_warn("Got visible plugin: %s", gel_plugin_stringify(plugin));
		// gel_warn("Added lock on %s", gel_plugin_stringify(plugin));

		iter = iter->next;
	}
	self->visible_plugins = g_list_reverse(self->visible_plugins);
	g_list_free(plugins);
}

static gint
plugin_usage_cmp(GelPlugin *a, GelPlugin *b)
{
	return gel_plugin_get_usage(a) - gel_plugin_get_usage(b);
}

static void
plugins_unload_plugins(EinaPlugins *self, gboolean all)
{
	// Remove locks on visible plugins
	g_list_foreach(self->visible_plugins, (GFunc) gel_plugin_remove_lock, NULL);
	g_list_free(self->visible_plugins);
	self->visible_plugins = NULL;

	GString *str = g_string_new(NULL);
	GList *plugins = g_list_sort(gel_app_query_plugins(eina_obj_get_app(self)), (GCompareFunc) plugin_usage_cmp);
	GList *iter = plugins; // self->visible_plugins;
	while (iter)
	{
		GelPlugin *plugin = (GelPlugin*) iter->data;

		if (gel_plugin_get_pathname(plugin) && gel_plugin_is_enabled(plugin))
			g_string_append_printf(str, "%s,", gel_plugin_get_pathname(plugin));
		if (all)
			gel_app_unload_plugin(eina_obj_get_app(self), plugin, NULL);
		
		iter = iter->next;
	}
	if (str->len > 1 )
	{
		str->str[str->len - 1] = '\0';
		eina_conf_set_str(self->conf, "/plugins/enabled", str->str);
	}
	g_string_free(str, TRUE);

	// Purge
	gel_app_purge(eina_obj_get_app(self));
}

static void
plugins_build_treeview(EinaPlugins *self)
{
	GList *l = self->visible_plugins;
	gtk_notebook_set_current_page(eina_obj_get_typed(self, GTK_NOTEBOOK, "tabs"),
		(l && l->data) ? PLUGINS_TAB_PLUGINS : PLUGINS_TAB_NO_PLUGINS );
	gtk_notebook_set_show_tabs (eina_obj_get_typed(self, GTK_NOTEBOOK, "tabs"), FALSE);
	if (!l || !l->data)
		gtk_widget_hide(eina_obj_get_typed(self, GTK_WIDGET, "plugin-info-button"));
	else
		gtk_widget_show(eina_obj_get_typed(self, GTK_WIDGET, "plugin-info-button"));

	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);

		GtkTreeIter iter;
		gtk_list_store_append(self->model, &iter);

		// gchar *escape = g_markup_escape_text(gel_plugin_stringify(plugin), -1);
		// gchar *markup = g_strdup_printf("<b>%s</b>\n%s\n<span foreground=\"grey\" size=\"small\">%s</span>",
		gchar *markup = g_strdup_printf("<big><b>%s</b></big>\n%s",
			plugin->name, plugin->short_desc);

		GError *err = NULL;
		GdkPixbuf *pb = NULL;
		gchar *tmp = gel_plugin_get_resource(plugin, GEL_RESOURCE_IMAGE, (gchar*) plugin->icon);
		if (!tmp)
		{
			gel_error(N_("Cannot locate resource '%s'"), plugin->icon);
		}
		else if ((pb = gdk_pixbuf_new_from_file_at_scale(tmp, 64, 64, TRUE, &err)) == NULL)
		{
			gel_error(N_("Cannot load resource '%s': '%s'"), tmp, err->message);
			g_error_free(err);
		}
		gel_free_and_invalidate(tmp, NULL, g_free);

		gtk_list_store_set(self->model, &iter,
			PLUGINS_COLUMN_ENABLED, gel_plugin_is_enabled(plugin),
			PLUGINS_COLUMN_ICON, pb,
			PLUGINS_COLUMN_MARKUP, markup,
			-1);
		
		g_free(markup);
		// g_free(escape);

		l = l->next;
	}
}

static void
plugins_show(EinaPlugins *self)
{
	plugins_load_plugins(self);
	plugins_build_treeview(self);

	g_signal_connect(eina_obj_get_app(EINA_OBJ(self)), "plugin-init",
		(GCallback) app_init_plugin_cb, self);
	g_signal_connect(eina_obj_get_app(EINA_OBJ(self)), "plugin-fini",
		(GCallback) app_fini_plugin_cb, self);
	gtk_dialog_run((GtkDialog *) self->window);
}

static void
plugins_hide(EinaPlugins *self)
{
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(self->treeview)));
	plugins_unload_plugins(self, FALSE);

	g_signal_handlers_disconnect_by_func(eina_obj_get_app(EINA_OBJ(self)),
		 app_init_plugin_cb, self);
	g_signal_handlers_disconnect_by_func(eina_obj_get_app(EINA_OBJ(self)),
		 app_fini_plugin_cb, self);
	gtk_widget_hide(self->window);
}

static GelPlugin*
plugins_get_active_plugin(EinaPlugins *self)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gint *indices;

	GtkTreeSelection *sel = gtk_tree_view_get_selection(self->treeview);
	if (!gtk_tree_selection_get_selected(sel, (GtkTreeModel **) &(self->model), &iter))
	{
		if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->model), &iter))
			return NULL;
	}

	if ((path = gtk_tree_model_get_path(GTK_TREE_MODEL(self->model), &iter)) == NULL)
	{
		gel_warn("Cannot get treepath from selection");
		plugins_hide(self);
		return NULL;
	}

	indices = gtk_tree_path_get_indices(path);
	return (self->active_plugin = (GelPlugin *) g_list_nth_data(self->visible_plugins, indices[0]));
}

static void
plugins_update_plugin_properties(EinaPlugins *self)
{
	GelPlugin *plugin;
	// gchar *tmp;

	if (self->active_plugin == NULL)
	{
		gel_info("No active plugin, no need to update properties");
		return;
	}
	plugin = self->active_plugin;
	#if 0
	tmp = g_strdup_printf("<span size=\"x-large\" weight=\"bold\">%s</span>", plugin->name);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "name-label"), tmp);
	g_free(tmp);

	tmp = (plugin->short_desc ? g_markup_escape_text(plugin->short_desc, -1) : NULL);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "short-desc-label"), tmp);
	gel_free_and_invalidate(tmp, NULL, g_free);

	tmp = (plugin->long_desc ? g_markup_escape_text(plugin->long_desc, -1) : NULL);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "long-desc-label"), tmp);
	gel_free_and_invalidate(tmp, NULL, g_free);

	tmp = (plugin->author ? g_markup_escape_text(plugin->author, -1) : NULL);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "author-label"), tmp);
	gel_free_and_invalidate(tmp, NULL, g_free);

	tmp = (plugin->url ? g_markup_escape_text(plugin->url, -1) : NULL);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "website-label"), tmp);
	gel_free_and_invalidate(tmp, NULL, g_free);

	GError *err = NULL;
	GdkPixbuf *pb = NULL;
	tmp = gel_plugin_get_resource(plugin, GEL_RESOURCE_IMAGE, (gchar*) plugin->icon);
	if ((tmp == NULL) ||
		!g_file_test(tmp, G_FILE_TEST_IS_REGULAR) ||
		((pb = gdk_pixbuf_new_from_file_at_scale(tmp, 64, 64, TRUE, &err)) == NULL))
	{
		if (err)
		{
			gel_error("Cannot load resource '%s': '%s'", tmp, err->message);
			g_error_free(err);
		}
		gtk_image_set_from_stock(eina_obj_get_typed(self, GTK_IMAGE, "icon-image"), "gtk-info", GTK_ICON_SIZE_MENU);
	}
	else
		gtk_image_set_from_pixbuf(eina_obj_get_typed(self, GTK_IMAGE, "icon-image"), pb);
	#endif
}

static void
update_enabled(EinaPlugins *self, GelPlugin *plugin)
{
	gint index;
	if ((index = g_list_index(self->visible_plugins, plugin)) == -1)
	{
		gel_warn("Unknow plugin (%s) initialized, ignoring", gel_plugin_stringify(plugin));
		return;
	}

	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(self->model), &iter, path);
	gtk_list_store_set(self->model, &iter,
		PLUGINS_COLUMN_ENABLED, gel_plugin_is_enabled(plugin), -1);
	gtk_tree_path_free(path);
}

// --
// Callbacks
// --

static void
app_init_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self)
{
	update_enabled(self, plugin);
}

static void
app_fini_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self)
{
	update_enabled(self, plugin);
}

static void
plugins_window_response_cb
(GtkDialog *dialog, gint response_id, EinaPlugins *self)
{
	GelPlugin *plugin = plugins_get_active_plugin(self);
	gel_warn("%p", plugin);
}

static void
plugins_treeview_cursor_changed_cb(GtkWidget *w, EinaPlugins *self)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	GelPlugin *plugin;
	gint *indices;

	GtkTreeSelection *sel = gtk_tree_view_get_selection(self->treeview);
	if (!gtk_tree_selection_get_selected(sel, (GtkTreeModel **) &(self->model), &iter))
	{
		if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->model), &iter))
			return;
	}

	if ((path = gtk_tree_model_get_path(GTK_TREE_MODEL(self->model), &iter)) == NULL)
	{
		gel_warn("Cannot get treepath from selection");
		plugins_hide(self);
		return;
	}

	indices = gtk_tree_path_get_indices(path);
	plugin = (GelPlugin *) g_list_nth_data(self->visible_plugins, indices[0]);
	if (self->active_plugin == plugin)
		return;

	self->active_plugin = plugin;
	plugins_update_plugin_properties(self);
}

static void
plugins_cell_renderer_toggle_toggled_cb
(GtkCellRendererToggle *w, gchar *path, EinaPlugins *self)
{
	GtkTreePath *_path;
	gint *indices;
	GelPlugin *plugin;

	if ((_path = gtk_tree_path_new_from_string(path)) == NULL)
	{
		gel_error("Cannot create GtkTreePath on plugin state toggle");
		return;
	}

	indices = gtk_tree_path_get_indices(_path);
	if ((plugin = g_list_nth_data(self->visible_plugins, indices[0])) == NULL)
	{
		gel_error("Cannot get corresponding plugin to row %d", indices[0]);
		gtk_tree_path_free(_path);
		return;
	}
	gtk_tree_path_free(_path);

	GError *err = NULL;
	if (gel_plugin_is_enabled(plugin))
	{
		gel_app_unload_plugin(eina_obj_get_app(self), plugin, &err);
	}
	else
	{
		gchar *dirname = g_path_get_dirname((gchar *) gel_plugin_get_pathname(plugin));
		gchar *plugin_name = g_path_get_basename(dirname);
		g_free(dirname);

		gel_app_load_plugin(eina_obj_get_app(self), (gchar *) gel_plugin_get_pathname(plugin), plugin_name, &err);
	}

	debug("%s of plugin %s: %s (%s)",
		gel_plugin_is_enabled(plugin) ? "Load" : "Unload",
		gel_plugin_stringify(plugin),
		err == NULL ? "successful" : "failed",
		err ? err->message : "none"
		);
}

static void
plugins_menu_activate_cb(GtkAction *action, EinaPlugins *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "Plugins"))
	{
		plugins_show(self);
	}
	else
	{
		gel_error("Action %s not defined", name);
	}
}
#endif
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

