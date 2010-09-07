/*
 * gel/gel-ui-plugin-manager.c
 *
 * Copyright (C) 2004-2010 GelUI
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

#include <string.h>
#include <glib/gi18n.h>
#include "gel-ui-plugin-manager.h"
#include "gel-ui-plugin-manager-ui.h"

G_DEFINE_TYPE (GelUIPluginManager, gel_ui_plugin_manager, GTK_TYPE_BOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_UI_TYPE_PLUGIN_MANAGER, GelUIPluginManagerPrivate))

typedef struct _GelUIPluginManagerPrivate GelUIPluginManagerPrivate;

struct _GelUIPluginManagerPrivate {
	GelPluginEngine *engine;

	GList       *infos;
	GtkTreeView *tv;
	GtkNotebook *tabs;
	GtkWidget   *info, *close;
};

enum {
	PROPERTY_APP = 1,
};

enum {
	COLUMN_ENABLED = 0,
	COLUMN_ICON,
	COLUMN_MARKUP,
	COLUMN_INFO
};

static void
set_engine(GelUIPluginManager *self, GelPluginEngine *engine);

static void
insert_info(GelUIPluginManager *self, GtkListStore *model, GtkTreeIter *iter, GelPluginInfo *info);
static void
update_info(GelUIPluginManager *self, GelPluginInfo *info, gboolean enabled);

static gboolean
get_iter_from_info(GelUIPluginManager *self, GtkTreeIter *iter, GelPluginInfo *info);

static void
enabled_renderer_toggled_cb(GtkCellRendererToggle *render, gchar *path, GelUIPluginManager *self);
static void
plugin_init_cb(GelPluginEngine *engine, GelPlugin *plugin, GelUIPluginManager *self);
static void
plugin_fini_cb(GelPluginEngine *engine, GelPlugin *plugin, GelUIPluginManager *self);

static void
gel_ui_plugin_manager_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	GelUIPluginManager *self = GEL_UI_PLUGIN_MANAGER(object);

	switch (property_id) {
	case PROPERTY_APP:
		g_value_set_object(value, (GObject *) gel_ui_plugin_manager_get_engine(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
gel_ui_plugin_manager_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	GelUIPluginManager *self = GEL_UI_PLUGIN_MANAGER(object);

	switch (property_id) {
	case PROPERTY_APP:
		set_engine(self, GEL_PLUGIN_ENGINE(g_value_get_object(value)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
gel_ui_plugin_manager_dispose (GObject *object)
{
	GelUIPluginManager *self = GEL_UI_PLUGIN_MANAGER(object);
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);
	
	if (priv->engine)
	{
		g_signal_handlers_disconnect_by_func(priv->engine, plugin_init_cb, self);
		g_signal_handlers_disconnect_by_func(priv->engine, plugin_fini_cb, self);
		priv->engine = NULL;
	}

	if (priv->infos)
	{
		gel_list_deep_free(priv->infos, gel_plugin_info_free);
		priv->infos = NULL;
	}

	G_OBJECT_CLASS (gel_ui_plugin_manager_parent_class)->dispose (object);
}

static void
gel_ui_plugin_manager_class_init (GelUIPluginManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelUIPluginManagerPrivate));

	object_class->get_property = gel_ui_plugin_manager_get_property;
	object_class->set_property = gel_ui_plugin_manager_set_property;
	object_class->dispose = gel_ui_plugin_manager_dispose;

	g_object_class_install_property(object_class, PROPERTY_APP,
		g_param_spec_object("engine", "Gel Plugin Engine", "GelPluginEngine object to handleto display",
		GEL_TYPE_PLUGIN_ENGINE, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));
}

static void
gel_ui_plugin_manager_init (GelUIPluginManager *self)
{
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);

	// Build UI
	gchar *objs[] = { "main-widget", "liststore", NULL };
	GError *error = NULL;
	GtkBuilder *builder = gtk_builder_new();
	if (gtk_builder_add_objects_from_string(builder, __gel_ui_plugin_manager_ui_xml, -1, objs, &error) == 0)
	{
		g_warning("%s", error->message);
		g_error_free(error);
		return;
	}

	gtk_box_pack_start(
		GTK_BOX(self),
		GTK_WIDGET(gtk_builder_get_object(builder, "main-widget")),
		TRUE, TRUE, 0);

	priv->tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
	priv->tabs = GTK_NOTEBOOK(gtk_builder_get_object(builder, "main-widget"));

	g_signal_connect(gtk_builder_get_object(builder, "enabled-renderer"), "toggled", (GCallback) enabled_renderer_toggled_cb, self);

	g_object_unref(builder);

	// priv->info  = gtk_dialog_add_button((GtkDialog *) self, GTK_STOCK_INFO, GEL_UI_PLUGIN_MANAGER_RESPONSE_INFO);
	// priv->close = gtk_dialog_add_button((GtkDialog *) self, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
}

GelUIPluginManager*
gel_ui_plugin_manager_new (GelPluginEngine *engine)
{
	return g_object_new (GEL_UI_TYPE_PLUGIN_MANAGER, "engine", engine, NULL);
}

GelPluginEngine *
gel_ui_plugin_manager_get_engine(GelUIPluginManager *self)
{
	return GET_PRIVATE(self)->engine;
}

static void
set_engine(GelUIPluginManager *self, GelPluginEngine *engine)
{
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv->engine == NULL);
	g_return_if_fail(engine != NULL);

	priv->engine = engine;

	// Get Infos
	gel_list_deep_free(priv->infos, gel_plugin_info_free);

	gel_plugin_engine_scan_plugins(priv->engine);
	priv->infos = g_list_sort(gel_plugin_engine_query_plugins(priv->engine), (GCompareFunc) gel_plugin_info_cmp);

	// Tabs
	gtk_notebook_set_show_tabs(priv->tabs, FALSE);
	if (!priv->infos)
	{
		gtk_notebook_set_current_page(priv->tabs, 0);
		return;
	}
	gtk_notebook_set_current_page(priv->tabs, 1);

	// Fill notebook
	GtkTreeIter   iter;
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(priv->tv));
	GList *l = priv->infos;
	while (l)
	{
		GelPluginInfo *info = (GelPluginInfo *) l->data;

		if (info->pathname)
		{
			gtk_list_store_append(model, &iter);
			insert_info(self, model, &iter, (GelPluginInfo *) l->data);
		}

		l = l->next;
	}

	// Watch in/out plugins
	g_signal_connect(engine,  "plugin-init", (GCallback) plugin_init_cb, self);
	g_signal_connect(engine,  "plugin-fini", (GCallback) plugin_fini_cb, self);
}


GelPlugin *
gel_ui_plugin_manager_get_selected_plugin(GelUIPluginManager *self)
{
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);

	GtkTreeSelection *tvsel;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GelPlugin    *plugin;

	tvsel = gtk_tree_view_get_selection(priv->tv);
	if (!gtk_tree_selection_get_selected(tvsel, &model, &iter))
		return NULL;

	gtk_tree_model_get(model, &iter, COLUMN_INFO, &plugin, -1);
	return plugin;
}

static gboolean
get_iter_from_info(GelUIPluginManager *self, GtkTreeIter *iter, GelPluginInfo *info)
{
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);
	GtkTreeModel *model = gtk_tree_view_get_model(priv->tv);
	if (!gtk_tree_model_get_iter_first(model, iter))
		return FALSE;
	do
	{
		GelPluginInfo *test;
		gtk_tree_model_get(model, iter, COLUMN_INFO, &test, -1);
		if (gel_plugin_info_equal(test, info))
			return TRUE;

	} while (gtk_tree_model_iter_next(model, iter));
	return FALSE;
}

static void
insert_info(GelUIPluginManager *self, GtkListStore *model, GtkTreeIter *iter, GelPluginInfo *info)
{
	gchar *pb_path = info->icon_pathname;
	GdkPixbuf *pb = NULL;

	if (info->icon_pathname)
		pb = gdk_pixbuf_new_from_file_at_size(info->icon_pathname, 64, 64, NULL);

	if (pb == NULL)
	{
		pb_path = gel_resource_locate(GEL_RESOURCE_IMAGE, "plugin.png");
		pb = gdk_pixbuf_new_from_file_at_size(pb_path, 64, 64, NULL);
		g_free(pb_path);
	}

	gchar *markup = g_strdup_printf("<span size='x-large' weight='bold'>%s</span>\n%s",
		info->name,
		info->short_desc ? info->short_desc : ""
		);

	gtk_list_store_set(model, iter,
		COLUMN_ENABLED, (gel_plugin_engine_get_plugin(GET_PRIVATE(self)->engine, info) != NULL), 
		COLUMN_ICON,    pb,
		COLUMN_MARKUP,  markup,
		COLUMN_INFO,    info,
		-1);
	g_free(markup);
}

static void
update_info(GelUIPluginManager *self, GelPluginInfo *info, gboolean enabled)
{
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);

	GtkTreeIter iter;
	if (!get_iter_from_info(self, &iter, info))
	{
		g_warning(N_("Loaded unknow plugin: %p"), info);
		return;
	}

	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(priv->tv));
	gtk_list_store_set(model, &iter, COLUMN_ENABLED, enabled, -1);
}

static void
enabled_renderer_toggled_cb(GtkCellRendererToggle *render, gchar *path, GelUIPluginManager *self)
{
	GelUIPluginManagerPrivate *priv = GET_PRIVATE(self);

	GtkTreePath *tp = gtk_tree_path_new_from_string(path);
	GtkTreeIter iter;

	GtkTreeModel *model = gtk_tree_view_get_model(priv->tv);
	gtk_tree_model_get_iter(model, &iter, tp);
	gtk_tree_path_free(tp);

	GelPluginInfo *info;
	gtk_tree_model_get(model, &iter,
		COLUMN_INFO, &info,
		-1);

	GelPlugin *plugin = gel_plugin_engine_get_plugin(priv->engine, info);
	if (plugin)
	{
		gel_plugin_engine_unload_plugin(priv->engine, plugin, NULL);
	}
	else
	{
		GError *error = NULL;
		if (!gel_plugin_engine_load_plugin(priv->engine, info, &error))
		{
			g_warning(N_("Cannot load plugin %s: %s"), info->name, error->message);
			g_error_free(error);
		}
	}
}

static void
plugin_init_cb(GelPluginEngine *engine, GelPlugin *plugin, GelUIPluginManager *self)
{
	update_info(self, (GelPluginInfo *) gel_plugin_get_info(plugin), TRUE);
}

static void
plugin_fini_cb(GelPluginEngine *engine, GelPlugin *plugin, GelUIPluginManager *self)
{
	update_info(self, (GelPluginInfo *) gel_plugin_get_info(plugin), FALSE);
}

