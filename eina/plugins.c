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

#include <gmodule.h>
#include <config.h>
#include <gel/gel-ui.h>
#include <eina/settings.h>
#include <eina/player.h>
#include <eina/plugins.h>

struct _EinaPlugins {
	EinaObj         parent;
	EinaConf       *conf;
	GtkWidget      *window;
	GtkTreeView    *treeview;
	GtkListStore   *model;
	GList          *plugins;
	GList          *all_plugins;
	GelPlugin      *active_plugin;
	GtkActionEntry *ui_actions;
	GtkActionGroup *ui_mng_ag;
	guint           ui_mng_merge_id;
};

enum {
	PLUGINS_COLUMN_ENABLED,
	PLUGINS_COLUMN_NAME,

	PLUGINS_N_COLUMNS
};

static void
app_init_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self);
static void
app_fini_plugin_cb(GelApp *app, GelPlugin *plugin, EinaPlugins *self);
static void
plugins_free_plugins(EinaPlugins *self);
static void
plugins_show(EinaPlugins *self);
static void
plugins_hide(EinaPlugins *self);
static void
plugins_treeview_fill(EinaPlugins *self);
static void
plugins_treeview_cursor_changed_cb(GtkWidget *w, EinaPlugins *self);
static void
plugins_cell_renderer_toggle_toggled_cb(GtkCellRendererToggle *w, gchar *path, EinaPlugins *self);
static void
plugins_menu_activate_cb(GtkAction *action, EinaPlugins *self);

//--
// Init/Exit functions 
//--
static gboolean
plugins_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self =  g_new0(EinaPlugins, 1);

	if (!eina_obj_init(EINA_OBJ(self), app, "plugins", EINA_OBJ_GTK_UI, error))
	{
		g_free(self);
		return FALSE;
	}

	// Load settings
	if ((self->conf = eina_obj_require(EINA_OBJ(self), "settings", error)) == NULL)
	{
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	// Build the GtkTreeView
	GtkTreeViewColumn *tv_col_enabled, *tv_col_name;
	GtkCellRenderer *enabled_render, *name_render;

	self->treeview = eina_obj_get_typed(self, GTK_TREE_VIEW, "treeview");
	self->model = gtk_list_store_new(PLUGINS_N_COLUMNS,
		G_TYPE_BOOLEAN,
		G_TYPE_STRING);
	gtk_tree_view_set_model(self->treeview, GTK_TREE_MODEL(self->model));

	enabled_render = gtk_cell_renderer_toggle_new();
	tv_col_enabled = gtk_tree_view_column_new_with_attributes(_("Enabled"),
		enabled_render, "active", PLUGINS_COLUMN_ENABLED,
		NULL);
	name_render = gtk_cell_renderer_text_new();
	tv_col_name    = gtk_tree_view_column_new_with_attributes(_("Plugin"),
		name_render, "markup", PLUGINS_COLUMN_NAME,
		NULL);

	gtk_tree_view_append_column(self->treeview, tv_col_enabled);
	gtk_tree_view_append_column(self->treeview, tv_col_name);

    g_object_set(G_OBJECT(name_render),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_MIDDLE,
		NULL);
	g_object_set(G_OBJECT(tv_col_enabled),
		"visible",   TRUE,
		"resizable", FALSE,
		NULL);
	g_object_set(G_OBJECT(tv_col_name),
		"visible",   TRUE,
		"resizable", FALSE,
		NULL);
	g_object_set(G_OBJECT(self->treeview),
		"headers-clickable", FALSE,
		"headers-visible", TRUE,
		NULL);

	// Setup signals
	self->window = eina_obj_get_widget(self, "main-window");

	g_signal_connect_swapped(self->window, "delete-event",
	G_CALLBACK(plugins_hide), self);

	g_signal_connect_swapped(self->window, "response",
	G_CALLBACK(plugins_hide), self);

	g_signal_connect(self->treeview, "cursor-changed",
	G_CALLBACK(plugins_treeview_cursor_changed_cb), self);

	g_signal_connect(enabled_render, "toggled",
	G_CALLBACK(plugins_cell_renderer_toggle_toggled_cb), self);

	// Menu entry
	EinaPlayer *player = EINA_OBJ_GET_PLAYER(self);
	if (player == NULL)
	{
		gel_warn("Cannot access player");
		return FALSE;
	}

	GtkUIManager *ui_manager = eina_player_get_ui_manager(player);
	self->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager,
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Plugins\" action=\"Plugins\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, error);
	if (self->ui_mng_merge_id == 0)
	{
		eina_obj_unrequire(EINA_OBJ(self), "settings", NULL);
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	GtkActionEntry action_entries[] = {
		{ "Plugins", NULL, N_("Plugins"),
	    NULL, NULL, G_CALLBACK(plugins_menu_activate_cb) }
	};
	
	self->ui_mng_ag = gtk_action_group_new("plugins");
	gtk_action_group_add_actions(self->ui_mng_ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(ui_manager, self->ui_mng_ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);

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
			if (!plugin)
			{
				gel_error("Cannot load plugin from path '%s': %s", plugins[i], err->message);
				gel_free_and_invalidate(err, NULL, g_error_free);
				continue;
			}

		}
		g_strfreev(plugins);
	}

	return TRUE;
}

static gboolean
plugins_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlugins *self = GEL_APP_GET_PLUGINS(app);

	plugins_free_plugins(self);
	
	// Unmerge menu
	GtkUIManager *ui_mng;
	if ((ui_mng = eina_player_get_ui_manager(EINA_OBJ_GET_PLAYER(self))) != NULL)
	{
		gtk_ui_manager_remove_action_group(ui_mng,
			self->ui_mng_ag);
		gtk_ui_manager_remove_ui(ui_mng,
			self->ui_mng_merge_id);
		g_object_unref(self->ui_mng_ag);
	}
	gtk_widget_destroy(self->window);

	eina_obj_unrequire(EINA_OBJ(self), "settings", NULL);
	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

//--
// Private functions
//--
static void
plugins_free_plugins(EinaPlugins *self)
{
	if (self->all_plugins == NULL)
		return;

	GList *iter = self->all_plugins;
	GList *dump = NULL;

	while (iter)
	{
		GelPlugin *plugin = GEL_PLUGIN(iter->data);
		if (gel_plugin_is_enabled(plugin))
		{
			dump = g_list_prepend(dump, plugin);
			iter = iter->next;
			continue;
		}

		gel_plugin_unref(plugin);
		iter = iter->next;
	}
	gel_free_and_invalidate(self->plugins, NULL, g_list_free);
	gel_free_and_invalidate(self->all_plugins, NULL, g_list_free);

	dump = g_list_reverse(dump);
	GString *dump_str = g_string_new(NULL);
	iter = dump;
	while (iter)
	{
		dump_str = g_string_append(dump_str, gel_plugin_get_pathname(GEL_PLUGIN(iter->data)));
		if (iter->next)
			dump_str = g_string_append_c(dump_str, ',');
		iter = iter->next;
	}
	g_list_free(dump);
	eina_conf_set_str(self->conf, "/plugins/enabled", dump_str->str);
	g_string_free(dump_str, TRUE);
}

static void
plugins_show(EinaPlugins *self)
{
	plugins_treeview_fill(self);
	g_signal_connect(eina_obj_get_app(EINA_OBJ(self)), "plugin-init",
		(GCallback) app_init_plugin_cb, self);
	g_signal_connect(eina_obj_get_app(EINA_OBJ(self)), "plugin-fini",
		(GCallback) app_fini_plugin_cb, self);
	gtk_widget_show(self->window);
}

static void
plugins_hide(EinaPlugins *self)
{
	plugins_free_plugins(self);
	g_signal_handlers_disconnect_by_func(eina_obj_get_app(EINA_OBJ(self)),
		 app_init_plugin_cb, self);
	g_signal_handlers_disconnect_by_func(eina_obj_get_app(EINA_OBJ(self)),
		 app_fini_plugin_cb, self);
	gtk_widget_hide(self->window);
}

static void
plugins_treeview_fill(EinaPlugins *self)
{
	GelApp *app = eina_obj_get_app(EINA_OBJ(self));
	GList *l;
	GtkTreeIter iter;

	gtk_list_store_clear(self->model);

	self->all_plugins = l = gel_app_query_plugins(app);
	while (l)
	{
		GelPlugin *plugin = GEL_PLUGIN(l->data);

		// Skip internal plugins
		if (gel_plugin_get_pathname(plugin) == NULL)
		{
			l = l->next;
			continue;
		}
		self->plugins = g_list_prepend(self->plugins, plugin);

		gtk_list_store_append(self->model, &iter);

		gchar *escape = g_markup_escape_text(gel_plugin_stringify(plugin), -1);
		gchar *markup = g_strdup_printf("<b>%s</b>\n%s\n<span foreground=\"grey\" size=\"small\">%s</span>",
			plugin->name, plugin->short_desc, escape);

		gtk_list_store_set(self->model, &iter,
			PLUGINS_COLUMN_ENABLED, gel_plugin_is_enabled(plugin),
			PLUGINS_COLUMN_NAME, markup,
			-1);

		g_free(markup);
		g_free(escape);

		l = l->next;
	}
	self->plugins = g_list_reverse(self->plugins);
}

static void
plugins_update_plugin_properties(EinaPlugins *self)
{
	GelPlugin *plugin;
	gchar *tmp;

	if (self->active_plugin == NULL)
	{
		gel_info("No active plugin, no need to update properties");
		return;
	}
	plugin = self->active_plugin;
	
	tmp = g_strdup_printf("<span size=\"x-large\" weight=\"bold\">%s</span>", plugin->name);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "name-label"), tmp);
	g_free(tmp);

	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "short-desc-label"), plugin->short_desc);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "long-desc-label"), plugin->long_desc);

	tmp = g_markup_escape_text(plugin->author, -1);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "author-label"), tmp);
	g_free(tmp);

	tmp = g_markup_escape_text(plugin->url, -1);
	gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "website-label"), tmp);
	g_free(tmp);

	gel_warn("Icon feature disabled");
	/*
	tmp = gel_plugin_build_resource_path(plugin, (gchar*) plugin->icon);
	if (!g_file_test(tmp, G_FILE_TEST_IS_REGULAR))
		gtk_image_set_from_stock(eina_obj_get_typed(self, GTK_IMAGE, "icon-image"), "gtk-info", GTK_ICON_SIZE_MENU);
	else
		gtk_image_set_from_file(eina_obj_get_typed(self, GTK_IMAGE, "icon-image"), tmp);
	g_free(tmp);
	*/
	gtk_image_set_from_stock(eina_obj_get_typed(self, GTK_IMAGE, "icon-image"), "gtk-info", GTK_ICON_SIZE_MENU);
}

static void
update_enabled(EinaPlugins *self, GelPlugin *plugin)
{
	gint index;
	gel_warn("Updating %s", plugin->name);
	if ((index = g_list_index(self->plugins, plugin)) == -1)
	{
		gel_warn("Unknow plugin initialized, ignoring");
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
	plugin = (GelPlugin *) g_list_nth_data(self->plugins, indices[0]);
	gel_warn("Selected plugin is: %s at state %d, usage %d", plugin->name, gel_plugin_is_enabled(plugin), gel_plugin_get_usage(plugin));
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
	GError *error = NULL;

	if ((_path = gtk_tree_path_new_from_string(path)) == NULL)
	{
		gel_error("Cannot create GtkTreePath on plugin state toggle");
		return;
	}

	indices = gtk_tree_path_get_indices(_path);
	if ((plugin = g_list_nth_data(self->plugins, indices[0])) == NULL)
	{
		gel_error("Cannot get corresponding plugin to row %d", indices[0]);
		gtk_tree_path_free(_path);
		return;
	}
	gtk_tree_path_free(_path);

	gboolean success;
	gel_warn("Selected plugin is: %s at state %d, usage %d", plugin->name, gel_plugin_is_enabled(plugin), gel_plugin_get_usage(plugin));

	if (gel_plugin_is_enabled(plugin))
	{
		if (gel_plugin_get_usage(plugin) > 1)
			gel_plugin_unref(plugin);

		if (gel_plugin_get_usage(plugin) == 1)
			success = gel_plugin_fini(plugin, &error);
		else
			success = TRUE;
	}
	else
	{
		if ((success = gel_plugin_init(plugin, &error)) == TRUE)
			gel_plugin_ref(plugin);
	}

	if (!success)
	{
		gel_warn("Got error: %s", error->message);
		g_error_free(error);
	}
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

G_MODULE_EXPORT GelPlugin plugins_plugin = {
	GEL_PLUGIN_SERIAL,
	"plugins", PACKAGE_VERSION,
	N_("Build-in plugin manager plugin"), NULL,
	NULL, NULL, NULL,
	plugins_init, plugins_fini,
	NULL, NULL
};

