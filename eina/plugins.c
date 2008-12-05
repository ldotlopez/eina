#define GEL_DOMAIN "Eina::Plugins"

#include "base.h"
#include "plugins.h"
#include <gmodule.h>
#include <gel/gel-ui.h>
#include <eina/loader.h>
#include <eina/plugin.h>

struct _EinaPlugins {
	EinaBase        parent;
	EinaConf       *conf;
	GtkWidget      *window;
	GtkTreeView    *treeview;
	GtkListStore   *model;
	GList          *plugins;
	EinaPlugin     *active_plugin;
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
G_MODULE_EXPORT gboolean eina_plugins_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlugins *self;

	self = g_new0(EinaPlugins, 1);
	if (!eina_base_init((EinaBase *) self, hub, "plugins", EINA_BASE_GTK_UI))
	{
		gel_error("Cannot create component");
		return FALSE;
	}

	// Load settings
	if ((self->conf = eina_base_require(EINA_BASE(self), "settings")) == NULL)
	{
		gel_error("Cannot load settings module");
		eina_base_fini(EINA_BASE(self));
		return FALSE;
	}

	// Build the GtkTreeView
	GtkTreeViewColumn *tv_col_enabled, *tv_col_name;
	GtkCellRenderer *enabled_render, *name_render;

	self->treeview = GTK_TREE_VIEW(W(self, "treeview"));
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
	plugins_treeview_fill(self);

	// Setup signals
	self->window = W(self, "main-window");

	g_signal_connect_swapped(self->window, "delete-event",
	G_CALLBACK(plugins_hide), self);

	g_signal_connect_swapped(self->window, "response",
	G_CALLBACK(plugins_hide), self);

	g_signal_connect(self->treeview, "cursor-changed",
	G_CALLBACK(plugins_treeview_cursor_changed_cb), self);

	g_signal_connect(enabled_render, "toggled",
	G_CALLBACK(plugins_cell_renderer_toggle_toggled_cb), self);

	// Menu entry
	EinaPlayer *player = EINA_BASE_GET_PLAYER(self);
	if (player == NULL)
	{
		gel_warn("Cannot access player");
		return TRUE;
	}

	GtkUIManager *ui_manager = eina_player_get_ui_manager(player);
	GError *error = NULL;
	self->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager,
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Plugins\" action=\"Plugins\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, &error);
	if (self->ui_mng_merge_id == 0)
	{
		gel_warn("Cannot merge UI: '%s'", error->message);
		g_error_free(error);
		return TRUE;
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
			EinaPlugin *plugin = eina_loader_load_plugin(EINA_BASE_GET_LOADER(self), plugins[i], NULL);
			if (!plugin)
				continue;
			if (!eina_plugin_init(plugin))
			{
				eina_loader_unload_plugin(EINA_BASE_GET_LOADER(self), plugin, NULL);
				continue;
			}
			
			self->plugins = g_list_prepend(self->plugins, plugin);
		}
		g_strfreev(plugins);
		self->plugins = g_list_reverse(self->plugins);
	}

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_plugins_exit
(gpointer data)
{
	EinaPlugins *self = (EinaPlugins *) data;

	// Unmerge menu
	GtkUIManager *ui_mng;
	if ((ui_mng = eina_player_get_ui_manager(EINA_BASE_GET_PLAYER(self))) != NULL)
	{
		gtk_ui_manager_remove_action_group(ui_mng,
			self->ui_mng_ag);
		gtk_ui_manager_remove_ui(ui_mng,
			self->ui_mng_merge_id);
		g_object_unref(self->ui_mng_ag);
	}

	gtk_widget_destroy(self->window);

	gel_hub_unload(HUB(self), "settings");
	eina_base_fini((EinaBase *) self);

	return TRUE;
}

//--
// Private functions
//--
static void
plugins_show(EinaPlugins *self)
{
	plugins_treeview_fill(self);
	gtk_widget_show(self->window);
}

static void
plugins_hide(EinaPlugins *self)
{
	EinaLoader *loader;
	GList *l;

	gtk_widget_hide(self->window);
	if ((loader = EINA_BASE_GET_LOADER(self)) == NULL)
		return;

	l = self->plugins;
	while (l)
	{
		EinaPlugin *plugin = (EinaPlugin *) l->data;
		if (!eina_plugin_is_enabled(plugin))
			eina_loader_unload_plugin(loader, plugin, NULL);
		l = l->next;
	}
	self->plugins = NULL;
}

static void
plugins_treeview_fill(EinaPlugins *self)
{
	EinaLoader *loader;
	GList *l;
	GtkTreeIter iter;

	if ((loader = EINA_BASE_GET_LOADER(self)) == NULL)
		return;

	gtk_list_store_clear(self->model);

	self->plugins = l = eina_loader_query_plugins(loader);
	while (l)
	{
		EinaPlugin *plugin = (EinaPlugin *) l->data;
		gchar *markup;

		gtk_list_store_append(self->model, &iter);
		markup = g_strdup_printf("<b>%s</b>\n%s", plugin->name, plugin->short_desc);
		gtk_list_store_set(self->model, &iter,
			PLUGINS_COLUMN_ENABLED, eina_plugin_is_enabled(plugin),
			PLUGINS_COLUMN_NAME, markup,
			-1);
		g_free(markup);

		l = l->next;
	}
}

static void
plugins_update_plugin_properties(EinaPlugins *self)
{
	EinaPlugin *plugin;
	gchar *tmp;

	if (self->active_plugin == NULL)
	{
		gel_info("No active plugin, no need to update properties");
		return;
	}
	plugin = self->active_plugin;
	
	tmp = g_strdup_printf("<span size=\"x-large\" weight=\"bold\">%s</span>", plugin->name);
	gtk_label_set_markup(GTK_LABEL(W(self, "name-label")), tmp);
	g_free(tmp);

	gtk_label_set_markup(GTK_LABEL(W(self, "short-desc-label")), plugin->short_desc);
	gtk_label_set_markup(GTK_LABEL(W(self, "long-desc-label")), plugin->long_desc);

	tmp = g_markup_escape_text(plugin->author, -1);
	gtk_label_set_markup(GTK_LABEL(W(self, "author-label")), tmp);
	g_free(tmp);

	tmp = g_markup_escape_text(plugin->url, -1);
	gtk_label_set_markup(GTK_LABEL(W(self, "website-label")), tmp);
	g_free(tmp);

	tmp = eina_plugin_build_resource_path(plugin, (gchar*) plugin->icon);
	if (!g_file_test(tmp, G_FILE_TEST_IS_REGULAR))
		gtk_image_set_from_stock(GTK_IMAGE(W(self, "icon-image")), "gtk-info", GTK_ICON_SIZE_MENU);
	else
		gtk_image_set_from_file(GTK_IMAGE(W(self, "icon-image")), tmp);
	g_free(tmp);
}

// --
// Callbacks
// --
static void
plugins_treeview_cursor_changed_cb(GtkWidget *w, EinaPlugins *self)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	EinaPlugin *plugin;
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
	plugin = (EinaPlugin *) g_list_nth_data(self->plugins, indices[0]);
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
	GtkTreeIter iter;
	gint *indices;
	EinaPlugin *plugin;

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

	gboolean do_toggle = FALSE;
	if (gtk_cell_renderer_toggle_get_active(w) == FALSE)
		do_toggle = eina_plugin_init(plugin);
	else
		do_toggle = eina_plugin_fini(plugin);

	if (!do_toggle)
		return;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->model), &iter, path);
	gtk_list_store_set(self->model, &iter,
		PLUGINS_COLUMN_ENABLED, !gtk_cell_renderer_toggle_get_active(w),
		-1);

	// Update conf
	GList *l = self->plugins;
	GList *tmp = NULL;
	while (l)
	{
		if (eina_plugin_is_enabled(EINA_PLUGIN(l->data)))
			tmp = g_list_prepend(tmp, (gpointer) eina_plugin_get_pathname(EINA_PLUGIN(l->data)));
		l = l->next;
	}
	tmp = g_list_reverse(tmp);
	gchar *plugins_str = gel_list_join(",", tmp);
	g_list_free(tmp);
	eina_conf_set_str(self->conf, "/plugins/enabled", plugins_str);
	g_free(plugins_str);
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

G_MODULE_EXPORT GelHubSlave plugins_connector = {
	"plugins",
	&eina_plugins_init,
	&eina_plugins_exit
};

