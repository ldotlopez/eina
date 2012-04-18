/*
 * gel/gel-ui-utils.c
 *
 * Copyright (C) 2004-2011 Eina
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

#define GEL_DOMAIN "GelUI"

#include "gel-ui-utils.h"

#include <glib/gi18n.h>
#include "gel-ui.h"

GQuark
gel_ui_quark(void)
{
	GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("gel-ui");
	return ret;
}

/**
 * gel_ui_load_resource:
 * @ui_filename: Filename for the UI file
 * @error: Location for #GError or %NULL
 *
 * Searchs and loads in to a #GtkBuilder the UI file named @ui_filename.
 * The file is located via gel_resource_locate() function and the same
 * rules are applied here.
 *
 * Returns: (transfer full): The #GtkBuilder for @ui_filename or %NULL.
 */
GtkBuilder *
gel_ui_load_resource(gchar *ui_filename, GError **error)
{
	g_return_val_if_fail(ui_filename, NULL);

	g_return_val_if_fail(g_str_has_suffix(ui_filename, ".ui"), NULL);
	gchar *ui_pathname = gel_resource_locate(GEL_RESOURCE_TYPE_UI, ui_filename);

	GError *suberror = NULL;

	if (ui_pathname == NULL)
	{
		g_set_error(&suberror, gel_ui_quark(), GEL_UI_ERROR_RESOURCE_NOT_FOUND,
			_("Cannot locate resource '%s'"), ui_filename);
		gel_propagate_error_or_warm(error, suberror, "%s", suberror->message);
		return NULL;
	}

	GtkBuilder *ret = gtk_builder_new();
	if (gtk_builder_add_from_file(ret, ui_pathname, &suberror) == 0)
	{
		gel_propagate_error_or_warm(error, suberror, "%s", suberror->message);
		g_object_unref(ret);
		ret = NULL;
	}

	return ret;
}

/**
 * gel_ui_pixbuf_from_uri:
 * @uri: (transfer none): An URI
 *
 * Creates a #GdkPixbuf from an URI
 *
 * Returns: (transfer full): The new #GdkPixbuf or %NULL
 */
GdkPixbuf*
gel_ui_pixbuf_from_uri(const gchar *uri)
{
	g_return_val_if_fail(uri, NULL);

	GFile *f = g_file_new_for_uri(uri);
	GdkPixbuf *ret = gel_ui_pixbuf_from_file(f);
	g_object_unref(f);

	return ret;
}
/**
 * gel_ui_pixbuf_from_file:
 * @file: (transfer none): A #GFile
 *
 * Creates a #GdkPixbuf from a #GFile
 *
 * Returns: (transfer full): The new #GdkPixbuf or %NULL
 */
GdkPixbuf *
gel_ui_pixbuf_from_file(const GFile *file)
{
	g_return_val_if_fail(G_IS_FILE(file), NULL);

	GFile *f = (GFile *) file;

	GError *error = NULL;
	GInputStream *input = (GInputStream *) g_file_read(f, NULL, &error);

	if (input == NULL)
	{
		gchar *uri = g_file_get_uri(f);
		g_warning(_("Unable to read GFile('%s'): %s"), uri, error->message);
		g_free(uri);
		g_error_free(error);
		return NULL;
	}

	GdkPixbuf *ret = gel_ui_pixbuf_from_stream(input);
	g_object_unref(input);

	return ret;
}

/**
 * gel_ui_pixbuf_from_stream:
 * @stream: (transfer none): A #GInputStream
 *
 * Creates a #GdkPixbuf from a #GInputStream
 *
 * Returns: (transfer full): The new #GdkPixbuf or %NULL
 */
GdkPixbuf*
gel_ui_pixbuf_from_stream(const GInputStream *stream)
{
	g_return_val_if_fail(G_IS_INPUT_STREAM(stream), NULL);

	g_seekable_seek((GSeekable *) stream, 0, G_SEEK_SET, NULL, NULL);

	GError *error = NULL;
	GdkPixbuf *ret = gdk_pixbuf_new_from_stream((GInputStream *) stream, NULL, &error);
	if (ret == NULL)
	{
		g_warning(_("Cannot load image from input stream %p: %s"), stream, error->message);
		g_error_free(error);
		return NULL;
	}
	return ret;
}

/**
 * gel_ui_pixbuf_from_value:
 * @value: (transfer none): A #GValue
 *
 * Creates a #GdkPixbuf from a #GValue
 *
 * Returns: (transfer full): The new #GdkPixbuf or %NULL
 */
GdkPixbuf *
gel_ui_pixbuf_from_value(const GValue *value)
{
	g_return_val_if_fail(G_IS_VALUE(value), NULL);

	GType type = G_VALUE_TYPE(value);

	if (type == G_TYPE_STRING)
		return gel_ui_pixbuf_from_uri(g_value_get_string(value));

	else if (type == G_TYPE_FILE)
		return gel_ui_pixbuf_from_file((GFile *) g_value_get_object(value));

	else if (type == G_TYPE_INPUT_STREAM)
		return gel_ui_pixbuf_from_stream((GInputStream *) g_value_get_object(value));

	else
	{
		g_warning(_("Unable to load image buffer from type '%s'. Please file a bug"), g_type_name(type));
		return NULL;
	}
}

#define _ui_signal_connect_from_def(ui,def,data,ret) \
	do { \
		GObject *object = (GObject *) gtk_builder_get_object(ui, def.object);     \
		if ((object == NULL) || !G_OBJECT(object))                                \
		{                                                                         \
			g_warning(_("Can not find object '%s' at GelUI %p"), def.object, ui); \
			ret = FALSE;                                                          \
		}                                                                         \
		gpointer _data = def.data ? def.data : data;                              \
		if (def.swapped)                                                          \
			g_signal_connect_swapped(object, def.signal, def.callback, _data);    \
		else                                                                      \
			g_signal_connect(object, def.signal, def.callback, _data);            \
		ret = TRUE;                                                               \
	} while (0)

/*
 * gel_ui_builder_connect_signal_from_def:
 * @ui: A #GtkBuilder.
 * @def: A #GelSignalDef with signal definition
 * @data: (allow-none): Userdata to pass to callback.
 *                      Not used if def.data is not %NULL
 *
 * Connects the signal from @def on @ui parameter for g_signal_connect()
 * If def.data is %NULL, the @data parameter is used as an alternative userdata
 * for callback
 *
 * Returns: %TRUE is successful, %FALSE otherwise.
 */
gboolean
gel_ui_builder_connect_signal_from_def(GtkBuilder *ui,
	GelSignalDef def, gpointer data)
{
	g_return_val_if_fail(GTK_IS_BUILDER(ui), FALSE);
	// FIXME: Check other parameters

	gboolean ret;
	_ui_signal_connect_from_def(ui, def, data, ret);
	return ret;
}

/*
 * gel_ui_builder_connect_signal_from_def_multiple:
 * @ui: A #GtkBuilder.
 * @defs: An array of #GelSignalDef
 * @n_entries: Number of elements in @def
 * @data: (allow-none): Userdata to pass to callback.
 *                      Not used if def[x].data is not %NULL
 * @count: (out caller-allocates) (allow-none): Location for store the number
 *         of successful connected signals.
 *
 * Calls gel_ui_builder_connect_signal_from_def() for each signal definition
 * in def
 *
 * Returns: %TRUE is all signals were connected, %FALSE otherwise.
 */
gboolean
gel_ui_builder_connect_signal_from_def_multiple(GtkBuilder *ui,
	GelSignalDef defs[], guint n_entries, gpointer data, guint *count)
{
	guint _count = 0;
	for (guint i = 0; i < n_entries; i++)
	{
		gboolean ret;
		_ui_signal_connect_from_def(ui, defs[i], data, ret);
		if (ret)
			_count++;
	}
	return (_count == n_entries);
}

/**
 * gel_ui_load_pixbuf_from_imagedef:
 * @def: Image definition
 * @error: Location for #GError or %NULL
 *
 * Loads image from @def into a #GdkPixbuf. Pathname of the image is
 * resolved via gel_resource_locate(). If an error is ocurred @error
 * is filled with the error and %NULL is returned.
 *
 * Returns: (allow-none) (transfer full): The #GdkPixbuf or %NULL
 */
GdkPixbuf *
gel_ui_load_pixbuf_from_imagedef(GelUIImageDef def, GError **error)
{
	// FIXME: Check in parameters

	gchar *pathname = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, def.image);
	if (pathname == NULL)
	{
		g_set_error(error, gel_ui_quark(), GEL_UI_ERROR_RESOURCE_NOT_FOUND, _("Unable to locate %s"), def.image);
		return NULL;
	}

	GError *suberror = NULL;
	GdkPixbuf *ret = gdk_pixbuf_new_from_file_at_size(pathname, def.w, def.h, &suberror);
	if (!ret)
		gel_propagate_error_or_warm(error, suberror, "%s", suberror->message);
	g_free(pathname);

	return ret;
}

/**
 * gel_ui_container_clear:
 * @container: A #GtkContainer
 *
 * Removes all children from @container using gtk_container_remove()
 */
void
gel_ui_container_clear(GtkContainer *container)
{
	g_return_if_fail(GTK_IS_CONTAINER(container));

	GList *l, *children = gtk_container_get_children(GTK_CONTAINER(container));
	for (l = children; l != NULL; l = l->next)
		gtk_container_remove((GtkContainer *) container, GTK_WIDGET(l->data));
	gel_free_and_invalidate(children, NULL, g_list_free);
}

/**
 * gel_ui_container_replace_children:
 * @container: A #GtkContainer
 * @widget: (transfer full): A #GtkWidget
 *
 * Removes all children from @container using gel_ui_container_clear() and
 * then packs @widget into @container
 */
void
gel_ui_container_replace_children(GtkContainer *container, GtkWidget *widget)
{
	g_return_if_fail(GTK_IS_CONTAINER(container));
	g_return_if_fail(GTK_IS_WIDGET(widget));

	gtk_container_foreach(container, (GtkCallback) gtk_widget_destroy, NULL);
	if (GTK_IS_BOX(container))
		gtk_box_pack_start(GTK_BOX(container), widget, TRUE, TRUE, 0);
	else if (GTK_IS_GRID(container))
	{
		gtk_grid_attach(GTK_GRID(container), widget, 0, 0, 1, 1);
		g_object_set((GObject *) widget, "hexpand", TRUE, "vexpand", TRUE, NULL);
	}
}


/**
 * gel_ui_container_find_widget:
 * @container: A #GtkContainer
 * @name: Child name
 *
 * Tries to find widget named @name in @container recursively. If it is not
 * found %NULL is returned. No references are added in this function.
 *
 * Returns: (transfer none) (allow-none): The widget
 */
GtkWidget *
gel_ui_container_find_widget(GtkContainer *container, const gchar *name)
{
	g_return_val_if_fail(GTK_IS_CONTAINER(container), NULL);
	g_return_val_if_fail(name, NULL);

	GtkWidget *ret = NULL;

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
			ret = gel_ui_container_find_widget((GtkContainer *) child, name);

		iter = iter->next;
	}

	return ret;
}

/**
 * gel_ui_tree_view_get_selected_indices:
 * @tv: A #GtkTreeView
 *
 * Returns a %NULL terminated array of gint with the indices of selected
 * rows, only works with a #GtkTreeView holding a #GtkListModel
 *
 * Returns: (allow-none) (transfer full): The selected indices, each index and the value
 *                           it self must be freeed with g_free
 */
gint *
gel_ui_tree_view_get_selected_indices(GtkTreeView *tv)
{
	g_return_val_if_fail(GTK_IS_TREE_VIEW(tv), NULL);
	g_return_val_if_fail(GTK_IS_LIST_STORE(gtk_tree_view_get_model(tv)), NULL);

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
			g_warning(_("Invalid GtkTreePath in selection, use %s only with ListModels"), __FUNCTION__);
			continue;
		}

		ret[i++] = indices[0];
		l = l->next;
	}
	g_list_foreach(l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(l);

	return ret;
}

/**
 * gel_ui_list_model_get_iter_from_index:
 * @model: A #GtkListStore
 * @iter: (out caller-allocates): A #GtkTreeIter
 * @index: An index to get the iter from
 *
 * Fills @iter with the #GtkTreeIter associated with index @index
 *
 * Returns: %TRUE if an iter is found, %FALSE otherwise
 */
gboolean
gel_ui_list_model_get_iter_from_index(GtkListStore *model, GtkTreeIter *iter, gint index)
{
	g_return_val_if_fail(GTK_IS_LIST_STORE(model), FALSE);
	g_return_val_if_fail(iter, FALSE);
	g_return_val_if_fail(index >= 0, FALSE);

	GtkTreePath *treepath = gtk_tree_path_new_from_indices(index, -1);
	gboolean ret = gtk_tree_model_get_iter(GTK_TREE_MODEL(model), iter, treepath);
	gtk_tree_path_free(treepath);

	g_warn_if_fail(ret);
	return ret;
}

/**
 * gel_ui_list_store_insert_at_index:
 * @model: A #GtkListStore
 * @index: Where to insert data
 * @Varargs: Data to insert
 *
 * Wrapper around gtk_list_store_insert()
 */
void
gel_ui_list_store_insert_at_index(GtkListStore *model, gint index, ...)
{
	g_return_if_fail(GTK_IS_LIST_STORE(model));
	g_return_if_fail(index >= 0);

	GtkTreeIter iter;
	gtk_list_store_insert(model, &iter, index);

	va_list args;
	va_start(args, index);
	gtk_list_store_set_valist(GTK_LIST_STORE(model), &iter, args);
	va_end(args);
}

/**
 * gel_ui_list_store_set_valist_at_index:
 * @model: A #GtkListStore
 * @index: Where to set data
 * @Varargs: Data to set
 *
 * Wrapper around gtk_list_store_set_valist()
 */
void
gel_ui_list_store_set_at_index(GtkListStore *model, gint index, ...)
{
	g_return_if_fail(GTK_IS_LIST_STORE(model));
	g_return_if_fail(index >= 0);

	va_list args;

	GtkTreeIter iter;
	g_return_if_fail(gel_ui_list_model_get_iter_from_index(model, &iter, index));

	va_start(args, index);
	gtk_list_store_set_valist(GTK_LIST_STORE(model), &iter, args);
	va_end(args);
}

/**
 * gel_ui_list_store_remove_at_index:
 * @model: A #GtkListStore
 * @index: Where is data to remove
 *
 * Wrapper around gtk_list_store_remove()
 */
void
gel_ui_list_store_remove_at_index(GtkListStore *model, gint index)
{
	g_return_if_fail(GTK_IS_LIST_STORE(model));
	g_return_if_fail(index >= 0);

	GtkTreeIter iter;
	g_return_if_fail(gel_ui_list_model_get_iter_from_index(model, &iter, index));

	gtk_list_store_remove(model, &iter);
}

/*
 * DnD
 */
enum {
//	TARGET_INT32,
	TARGET_STRING,
 	TARGET_ROOTWIN
};

static const GtkTargetEntry target_list[] = {
	// { "INTEGER",    0, TARGET_INT32 },
	{ "STRING",     0, TARGET_STRING },
	{ "text/plain", 0, TARGET_STRING },
	{ "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

typedef struct {
	gpointer user_data;
	GCallback callback;
} GelUIDndData;

static gboolean
__gel_ui_drag_drop_cb(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data)
{
	/* If the source offers a target */
	GList *targets = gdk_drag_context_list_targets(context);
	if (targets)
	{
		/* Choose the best target type */
		GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (targets, TARGET_STRING));

		/* Request the data from the source. */
		gtk_drag_get_data (
			widget,         /* will receive 'drag-data-received' signal */
			context,        /* represents the current state of the DnD */
			target_type,    /* the target type we want */
			time            /* time stamp */
			);
		return TRUE;
	}
	/* No target offered by source => error */
	else
		return FALSE;
}

static void
__gel_ui_drag_data_received (GtkWidget *widget,
	GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint target_type, guint time, gpointer data)
{
	gboolean success = FALSE;

	if ((selection_data == NULL) && (gtk_selection_data_get_length(selection_data) <= 0))
		success = FALSE;
	else
	{
		switch (target_type)
		{
		case TARGET_STRING:
			success = TRUE;
			break;
		default:
			g_warning("Unknow type");
			success = FALSE;
			break;
		}

		void (*callback) (GtkWidget *w, GType type, const guchar *data, gpointer user_data) =
			g_object_get_data((GObject *) widget, "gel-ui-dnd-callback");
		gpointer user_data = (gpointer)  g_object_get_data((GObject *) widget, "gel-ui-dnd-user-data");
		if (callback)
			callback(widget, G_TYPE_STRING, gtk_selection_data_get_data(selection_data), user_data);
	}
	gtk_drag_finish (context, success, FALSE, time);
}

/**
 * gel_ui_widget_enable_drop:
 * @widget: A #GtkWidget
 * @callback: (type gpointer): A #GCallback to be called on "drag-drop" or "drag-data-received" signals
 * @user_data: Data to pass to @callback
 *
 * This function automatizes some of the black magic of DnD on Gtk+.
 */
void
gel_ui_widget_enable_drop(GtkWidget *widget, GCallback callback, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));

	if (g_object_get_data((GObject *) widget, "gel-ui-dnd-callback") != NULL)
	{
		g_warning(_("Widget has been already made droppable by GelUI, ignoring."));
		return;
	}

	gtk_drag_dest_set (widget,
		GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
		target_list,
		G_N_ELEMENTS(target_list),
		GDK_ACTION_COPY
	);

	g_object_set_data((GObject *) widget, "gel-ui-dnd-callback",  (gpointer) callback);
	g_object_set_data((GObject *) widget, "gel-ui-dnd-user-data", (gpointer) user_data);

	g_signal_connect(widget, "drag-drop",          (GCallback) __gel_ui_drag_drop_cb, NULL);
	g_signal_connect(widget, "drag-data-received", (GCallback) __gel_ui_drag_data_received, NULL);
}

/**
 * gel_ui_widget_disable_drop:
 * @widget: A #GtkWidget
 *
 * Disables DnD functions added by gel_ui_widget_enable_drop on @widget
 */
void
gel_ui_widget_disable_drop(GtkWidget *widget)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));

	if (g_object_get_data((GObject *) widget, "gel-ui-dnd-callback") == NULL)
	{
		g_warning(_("Widget has not been made droppable by GelUI, ignoring."));
		return;
	}

	gtk_drag_dest_unset(widget);

	g_object_set_data((GObject *) widget, "gel-ui-dnd-callback",  NULL);
	g_object_set_data((GObject *) widget, "gel-ui-dnd-user-data", NULL);

	g_signal_handlers_disconnect_by_func(widget, __gel_ui_drag_drop_cb, NULL);
	g_signal_handlers_disconnect_by_func(widget, __gel_ui_drag_data_received, NULL);
}

