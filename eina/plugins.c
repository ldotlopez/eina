#define GEL_DOMAIN "Eina::Plugins"

#include "base.h"
#include "plugins.h"
#include <gmodule.h>
#include <gel/gel-ui.h>
#include "eina/iface.h"

struct _EinaPlugins {
	EinaBase      parent;
	GtkWidget    *window;
	GtkTreeView  *treeview;
	GtkListStore *model;
};

enum {
	PLUGINS_COLUMN_ENABLED,
	PLUGINS_COLUMN_NAME,

	PLUGINS_N_COLUMNS
};


static void
plugins_treeview_fill(EinaPlugins *self);

//--
// Signal definitions
//--
GelUISignalDef _plugins_signals[] = {
	// { "nameType", "something", G_CALLBACK(on_plugins_lomo_something) }, 
	GEL_UI_SIGNAL_DEF_NONE
};

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

	g_signal_connect(self->window, "delete-event",
	G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	g_signal_connect(self->window, "response",
	G_CALLBACK(gtk_widget_hide), NULL);

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

void
eina_plugins_show(EinaPlugins *self)
{
	plugins_treeview_fill(self);
	gtk_widget_show(self->window);
}

void
eina_plugins_hide(EinaPlugins *self)
{
	gtk_widget_hide(self->window);
}

//--
// Private functions
//--
static void
plugins_treeview_fill(EinaPlugins *self)
{
	EinaIFace *iface;
	GList *plugins, *l;
	GtkTreeIter iter;

	if ((iface = gel_hub_shared_get(HUB(self), "iface")) == NULL)
	{
		gel_error("Cannot get EinaIFace");
		return;
	}

	gtk_list_store_clear(self->model);

	plugins = l = eina_iface_list_available_plugins(iface);
	gel_warn("Got %d available plugins", g_list_length(plugins));
	while (l)
	{
		EinaPlugin *plugin = (EinaPlugin *) l->data;

		gel_warn("[%s] Path: %s",
			eina_plugin_is_enabled(plugin) ? "ENAB" : "DISA",
			plugin->name);

		gtk_list_store_append(self->model, &iter);
		gtk_list_store_set(self->model, &iter,
			PLUGINS_COLUMN_ENABLED, eina_plugin_is_enabled(plugin),
			PLUGINS_COLUMN_NAME, plugin->name,
			-1);
		if (!eina_plugin_is_enabled(plugin))
			eina_iface_unload_plugin(iface, plugin);
		l = l->next;
	}
	g_list_free(plugins);


}

G_MODULE_EXPORT GelHubSlave plugins_connector = {
	"plugins",
	&eina_plugins_init,
	&eina_plugins_exit
};

