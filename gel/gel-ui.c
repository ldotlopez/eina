/*
 * gel/gel-ui.c
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

#define GEL_DOMAIN "Gel"
#include "gel-ui.h"
#include <glib/gi18n.h>

static GQuark
gel_ui_quark(void)
{
	GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("gel-ui");
	return ret;
}

/*
 * UI creation
 */
GelUI *
gel_ui_load_resource(gchar *ui_filename, GError **error)
{
	GelUI *ret = NULL;
	gchar *ui_pathname;
	gchar *tmp;

	tmp = g_strconcat(ui_filename, ".ui", NULL);
	ui_pathname = gel_resource_locate(GEL_RESOURCE_UI, tmp);

	if (ui_pathname == NULL)
	{
		g_set_error(error, gel_ui_quark(), GEL_UI_ERROR_RESOURCE_NOT_FOUND,
			N_("Cannot locate resource '%s'"), tmp);
		g_free(tmp);
		return NULL;
	}
	g_free(tmp);

	ret = gtk_builder_new();
	if (gtk_builder_add_from_file(ret, ui_pathname, error) == 0) {
		gel_error("Error loading GtkBuilder: %s", (*error)->message);
		g_object_unref(ret);
		ret = NULL;
	}

	return ret;
}

/*
 * Signal handling
 */
gboolean
gel_ui_signal_connect_from_def(GelUI *ui, GelUISignalDef def, gpointer data, GError **error)
{
	GObject *object;

	object = (GObject *) gtk_builder_get_object(ui, def.widget);

	if ((object == NULL) || !G_OBJECT(object))
	{ 
		gel_warn("Can not find object '%s' at GelUI %p", def.widget, ui);
		return FALSE;
	}

	g_signal_connect(object, def.signal, def.callback, data);
	return TRUE;
}

gboolean
gel_ui_signal_connect_from_def_multiple(GelUI *ui, GelUISignalDef defs[], gpointer data, guint *count)
{
	guint _count = 0;
	gboolean ret = TRUE;
	guint i;
	
	for (i = 0; defs[i].widget != NULL; i++)
	{
		if (gel_ui_signal_connect_from_def(ui, defs[i], data, NULL))
			_count++;
		else
			ret = FALSE;
	}

	if (count != NULL)
		*count = _count;
	return ret;
}

/*
 * Images on UI's
 */
GdkPixbuf *
gel_ui_load_pixbuf_from_imagedef(GelUIImageDef def, GError **error)
{
	GdkPixbuf *ret;

	gchar *pathname = gel_plugin_get_resource(NULL, GEL_RESOURCE_IMAGE, def.image);
	if (pathname == NULL)
	{
		// Handle error
		return NULL;
	}

	ret = gdk_pixbuf_new_from_file_at_size(pathname, def.w, def.h, error);
	g_free(pathname);
	return ret;
}

void
gel_ui_container_clear(GtkContainer *container)
{
	GList *l, *children = gtk_container_get_children(GTK_CONTAINER(container));
	for (l = children; l != NULL; l = l->next)
		gtk_container_remove((GtkContainer *) container, GTK_WIDGET(l->data));
	gel_free_and_invalidate(children, NULL, g_list_free);
}

void
gel_ui_container_replace_children(GtkContainer *container, GtkWidget *widget)
{
	gtk_container_foreach(container, (GtkCallback) gtk_widget_destroy, NULL);
	gtk_box_pack_start(GTK_BOX(container), widget, TRUE, TRUE, 0); 
}


GtkWidget *
gel_ui_container_find_widget(GtkContainer *container, gchar *name)
{
	GtkWidget *ret = NULL;

	g_return_val_if_fail(GTK_IS_CONTAINER(container), NULL);
	g_return_val_if_fail(name, NULL);

	GList *children = gtk_container_get_children(container);
	GList *iter = children;
	while (iter && (ret == NULL))
	{
		GtkWidget *child = (GtkWidget *) iter->data;
		const gchar *c_name= gtk_buildable_get_name(GTK_BUILDABLE(child));

		if (c_name && g_str_equal(name, c_name))
		{
			ret = child;
			break;
		}

		if (GTK_IS_CONTAINER(child))
			ret = gel_ui_container_find_widget((GtkContainer *) child, (gchar *) name);

		iter = iter->next;
	}

	return ret;
}

//
// TreeStore/ListStore helpers
//
gint *
gel_ui_tree_view_get_selected_indices(GtkTreeView *tv)
{

	GtkTreeSelection *selection = gtk_tree_view_get_selection(tv);
	GtkTreeModel     *model     = gtk_tree_view_get_model(tv);

	GList *l, *iter; // iter over selected treepaths
	gint  *indices;  // hold treepath's indices
	gint i = 0;      // index
	l = iter = gtk_tree_selection_get_selected_rows(selection, &model);
	gint *ret = g_new0(gint, g_list_length(l) + 1);
	while (iter)
	{
		indices = gtk_tree_path_get_indices((GtkTreePath *) iter->data);
		if (!indices || !indices[0] || !indices[1] || (indices[1] != -1))
		{
			g_warning(N_("Invalid GtkTreePath in selection, use %s only with ListModels"), __FUNCTION__);
			continue;
		}

		ret[i++] = indices[0];
		l = l->next;
	}
	g_list_foreach(l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(l);

	return ret;
}

gboolean
gel_ui_list_model_get_iter_from_index(GtkListStore *model, GtkTreeIter *iter, gint index)
{
	GtkTreePath *treepath = gtk_tree_path_new_from_indices(index, -1);
	gboolean ret = gtk_tree_model_get_iter(GTK_TREE_MODEL(model), iter, treepath);
	gtk_tree_path_free(treepath);
	return ret;
}

void
gel_ui_list_store_insert_at_index(GtkListStore *model, gint index, ...)
{
	GtkTreeIter iter;
	gtk_list_store_insert(model, &iter, index);

	va_list args;
	va_start(args, index);
	gtk_list_store_set_valist(GTK_LIST_STORE(model), &iter, args);
	va_end(args);
}

void
gel_ui_list_store_set_valist_at_index(GtkListStore *model, gint index, ...)
{
	va_list args;

	GtkTreeIter iter;
	g_return_if_fail(gel_ui_list_model_get_iter_from_index(model, &iter, index));
		
	va_start(args, index);
	gtk_list_store_set_valist(GTK_LIST_STORE(model), &iter, args);
	va_end(args);
}

void
gel_ui_list_store_remove_at_index(GtkListStore *model, gint index)
{
	GtkTreeIter iter;
	g_return_if_fail(gel_ui_list_model_get_iter_from_index(model, &iter, index));

	gtk_list_store_remove(model, &iter);
}

G_END_DECLS
