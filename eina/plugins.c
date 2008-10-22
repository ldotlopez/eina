#define GEL_DOMAIN "Eina::Plugins"

#include "base.h"
#include "plugins.h"
#include <gmodule.h>
#include <gel/gel-ui.h>
#include "eina/iface.h"

struct _EinaPlugins {
	EinaBase        parent;
	GtkWidget      *window;
	GtkTreeView    *treeview;
	GtkListStore   *model;
	GList          *plugins;
	EinaPlugin     *active_plugin;
	GtkActionEntry *ui_actions;
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
	guint merge_id = gtk_ui_manager_add_ui_from_string(ui_manager,
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Plugins\" action=\"Plugins\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, &error);
	if (merge_id == 0)
	{
		gel_warn("Cannot merge UI: '%s'", error->message);
		g_error_free(error);
		return TRUE;
	}

	GtkActionEntry action_entries[] = {
		{ "Plugins", NULL, N_("Plugins"),
	    NULL, NULL, G_CALLBACK(plugins_menu_activate_cb) }
	};
	
	GtkActionGroup *ag = gtk_action_group_new("plugins");
	gtk_action_group_add_actions(ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(ui_manager, ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_plugins_exit
(gpointer data)
{
	EinaPlugins *self = (EinaPlugins *) data;

	gtk_widget_destroy(self->window);

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
	EinaIFace *iface;
	GList *l;

	gtk_widget_hide(self->window);
	if ((iface = gel_hub_shared_get(HUB(self), "iface")) == NULL)
		return;

	l = self->plugins;
	while (l)
	{
		EinaPlugin *plugin = (EinaPlugin *) l->data;
		if (!eina_plugin_is_enabled(plugin))
			eina_iface_unload_plugin(iface, plugin);
		l = l->next;
	}
	g_list_free(self->plugins);
	self->plugins = NULL;
}

static void
plugins_treeview_fill(EinaPlugins *self)
{
	EinaIFace *iface;
	GList *l;
	GtkTreeIter iter;

	if ((iface = gel_hub_shared_get(HUB(self), "iface")) == NULL)
		return;

	gtk_list_store_clear(self->model);

	self->plugins = l = eina_iface_list_available_plugins(iface);
	gel_warn("Got %d available plugins", g_list_length(self->plugins));
	while (l)
	{
		EinaPlugin *plugin = (EinaPlugin *) l->data;
		gchar *markup;

		gel_warn("[%s] Path: %s",
			eina_plugin_is_enabled(plugin) ? "ENAB" : "DISA",
			plugin->name);

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
	gel_warn("Active plugin: %p", plugin);
	plugins_update_plugin_properties(self);
}

static void
plugins_cell_renderer_toggle_toggled_cb
(GtkCellRendererToggle *w, gchar *path, EinaPlugins *self)
{
	EinaIFace *iface;
	GtkTreePath *_path;
	gint *indices;
	EinaPlugin *plugin;
	gboolean ret;

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

	if ((iface = gel_hub_shared_get(HUB(self), "iface")) == NULL)
	{
		gel_error("Cannot initialize plugin, cant access to EinaIFace");
		return;
	}

	gel_warn("New state: %d", gtk_cell_renderer_toggle_get_active(w));
	if (eina_plugin_is_enabled(plugin))
		ret = eina_iface_fini_plugin(iface, plugin);
	else
		ret = eina_iface_init_plugin(iface, plugin);
	gel_warn("Status: %d");
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

