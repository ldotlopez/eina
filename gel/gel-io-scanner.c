/*
 * gel/gel-io-scanner.c
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


#include "gel-io-scanner.h"
#include <string.h>
#include <gel/gel.h>
#include "gel-marshallers.h"

G_DEFINE_TYPE (GelIOScanner, gel_io_scanner, G_TYPE_OBJECT)

struct _GelIOScannerPrivate {
	GList    *uris;
	gchar    *attributes;
	gboolean  recurse;

	GList        *results;
	GQueue       *queue;
	GCancellable *cancellable;
};

static gboolean
_scanner_run_queue_idle_wrapper(gpointer self);
static void
_scanner_run_queue(GelIOScanner *self);

static void
_scanner_query_info_cb(GFile *source, GAsyncResult *res, GelIOScanner *self);
static void
_scanner_enumerate_children_cb(GFile *source, GAsyncResult *res, GelIOScanner *self);
static void
_scanner_enumerator_next_cb(GFileEnumerator *e, GAsyncResult *res, GelIOScanner *self);

static gboolean
_scanner_free_node(GNode *node, gpointer data);
static void
_scanner_sort_children(GNode *parent);
static gint
_scanner_cmp_by_type_by_name_cb(GNode *a, GNode *b);

static gboolean
_scanner_traverse_cb(GNode *node, GList **list);

enum {
	FINISH,
	ERROR,
	CANCEL,

	LAST_SIGNAL
};
static guint scanner_signals[LAST_SIGNAL] = { 0 }; 

static void
gel_io_scanner_dispose (GObject *object)
{
	GelIOScannerPrivate *priv = GEL_IO_SCANNER(object)->priv;

	gel_free_and_invalidate(priv->attributes, NULL, g_free);

	if (priv->cancellable)
	{
		g_cancellable_cancel(priv->cancellable);
		g_object_unref(priv->cancellable);
		priv->cancellable = NULL;
	}
	if (priv->queue)
	{
		g_queue_foreach(priv->queue, (GFunc) _scanner_free_node, NULL);
		g_queue_foreach(priv->queue, (GFunc) g_node_destroy, NULL);
		g_queue_free(priv->queue);
		priv->queue = NULL;
	}
	if (priv->results)
	{
		GList *iter = priv->results;
		while (iter)
		{
			g_node_traverse((GNode *) iter->data, G_IN_ORDER, G_TRAVERSE_ALL, -1, _scanner_free_node, NULL);
			g_node_destroy((GNode *) iter->data);
			iter = iter->next;
		}
		g_list_free(priv->results);
		priv->results = NULL;
	}

	G_OBJECT_CLASS (gel_io_scanner_parent_class)->dispose (object);
}

static void
gel_io_scanner_class_init (GelIOScannerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelIOScannerPrivate));

	/**
	 * GelIOScanner::finish:
	 * @scanner: The #GelIOScanner
	 * @results: Results of the operation
	 *
	 * Emitted after scan completion
	 */
	scanner_signals[FINISH] =
		g_signal_new ("finish",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (GelIOScannerClass, finish),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__POINTER,
			    G_TYPE_NONE,
				1,
				G_TYPE_POINTER);

	/**
	 * GelIOScanner::error:
	 * @scanner: The #GelIOScanner
	 * @error: A #GError
	 *
	 * Emitted if an error is encountered
	 */
	scanner_signals[ERROR] =
		g_signal_new ("error",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (GelIOScannerClass, error),
			    NULL, NULL,
			    gel_marshal_VOID__OBJECT_POINTER,
			    G_TYPE_NONE,
			    2,
				G_TYPE_OBJECT,
				G_TYPE_POINTER);

	/**
	 * GelIOScanner::cancel:
	 * @scanner: The #GelIOScanner
	 *
	 * Emitted if the scan if canceled
	 */
	scanner_signals[CANCEL] =
		g_signal_new ("cancel",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (GelIOScannerClass, cancel),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);

	object_class->dispose = gel_io_scanner_dispose;
}

static void
gel_io_scanner_init (GelIOScanner *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), GEL_IO_TYPE_SCANNER, GelIOScannerPrivate));
}

/**
 * gel_io_scanner_new:
 *
 * Creates a new #GelIOScanner
 *
 * Returns: (transfer full): The scanner
 */
GelIOScanner*
gel_io_scanner_new (void)
{
	return g_object_new (GEL_IO_TYPE_SCANNER, NULL);
}

/**
 * gel_io_scanner_new_full:
 *
 * @uris: (element-type utf8) (transfer none): URIs to scan (gchar*)
 * @attributes: Attributes to retrieve from URIs, see #GFileInfo attributes
 * @recurse: Recurse deep into the uris
 *
 * Scans URIs from @uris, retrieving the requested @attributes
 *
 * Returns: (transfer full): A new #GelIOScanner in progress
 */
GelIOScanner*
gel_io_scanner_new_full(GList *uris, const gchar *attributes, gboolean recurse)
{
	GelIOScanner *self = gel_io_scanner_new();
	gel_io_scanner_scan(self, uris, attributes, recurse);
	return self;
}

/**
 * gel_io_scanner_scan:
 *
 * @self: A existing #GelIOScanner
 * @uris: (element-type utf8) (transfer none): A #GList of URIs (gchar*) to * scan
 * @attributes: Attributes to retrieve from URIs, see #GFileInfo attributes
 * @recurse: Recurse deep into the uri
 *
 * Scans URIs on a existing #GelIOScanner. Fails if @self it is already on scan
 */
void
gel_io_scanner_scan(GelIOScanner *self, GList *uris, const gchar *attributes, gboolean recurse)
{
	g_return_if_fail(GEL_IO_IS_SCANNER(self));
	g_return_if_fail(uris != NULL);
	g_return_if_fail(attributes != NULL);
	g_return_if_fail(self->priv->queue == NULL);

	GelIOScannerPrivate *priv = self->priv;

	priv->attributes = g_strdup(attributes);
	priv->recurse    = recurse;
	// priv->success_cb = success_cb;
	// priv->error_cb   = error_cb;
	// priv->userdata   = data;

	priv->cancellable = g_cancellable_new();
	priv->queue = g_queue_new();
	GList *iter = uris;
	while (iter)
	{
		GFile *file = g_file_new_for_uri(iter->data);
		g_queue_push_tail(priv->queue, file);

		iter = iter->next;
	}

	g_idle_add(_scanner_run_queue_idle_wrapper, self);
}

/**
 * gel_io_scanner_flatten_result:
 * @forest: (transfer none): A #GList of #GNode to flat
 *
 * Flatens a forest (a list of trees) of results
 *
 * Returns: (transfer container): The forest in a 'flatten' state. Data from
 * forest is reutilized in the returned #GList. Data from forest is reutilized
 * in the returned #GList. Data from forest is reutilized in the returned
 * #GList. Data from forest is reutilized in the returned #GList
 */
GList*
gel_io_scanner_flatten_result(GList *forest)
{
	GList *ret = NULL;

	GList *iter = forest;
	while (iter)
	{
		g_node_traverse((GNode *) iter->data, G_PRE_ORDER, G_TRAVERSE_ALL, -1, (GNodeTraverseFunc) _scanner_traverse_cb, &ret);
		iter = iter->next;
	}
	return g_list_reverse(ret); 
}

static void
_scanner_query_info_cb(GFile *source, GAsyncResult *res, GelIOScanner *self)
{
	GelIOScannerPrivate *priv = self->priv;

	GError *error = NULL;
	GFileInfo *info = g_file_query_info_finish((GFile *) source, res, &error);
	if (info == NULL)
	{
		g_signal_emit(self, scanner_signals[ERROR], 0, source, error);
		g_error_free(error);
		g_object_unref(source);
		// _scanner_run_error(self, source, error);
		_scanner_run_queue(self);
		return;
	}

	// This GFile is already completed, add to results (it is a root)
	GNode *node = g_node_new(source);
	g_object_set_data((GObject *) source, "x-node",      node);
	g_object_set_data((GObject *) source, "g-file-info", info);
	priv->results = g_list_append(priv->results, node);

	GFileType type = g_file_info_get_file_type(info);
	if ((type == G_FILE_TYPE_DIRECTORY) && priv->recurse)
	{
		g_file_enumerate_children_async(source, priv->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
			priv->cancellable, (GAsyncReadyCallback) _scanner_enumerate_children_cb, self);
	}
	else if (type == G_FILE_TYPE_REGULAR)
	{
		_scanner_run_queue(self);
	}
	else
	{
		g_warning("Unknow file type for '%s': '%s'", "(fixme)", "(fixme)");
		_scanner_run_queue(self);
	}
}

static void
_scanner_enumerate_children_cb(GFile *source, GAsyncResult *res, GelIOScanner *self)
{
	GelIOScannerPrivate *priv = self->priv;
	GError *error = NULL;
	GFileEnumerator *e = g_file_enumerate_children_finish(source, res, &error);
	if (e == NULL)
	{
		g_signal_emit(self, scanner_signals[ERROR], 0, source, error);
		g_error_free(error);
		g_object_unref(source);
		// _scanner_run_error(self, source, error);
		_scanner_run_queue(self);
		return;
	}

	g_file_enumerator_next_files_async(e, 8, G_PRIORITY_DEFAULT,
		priv->cancellable, (GAsyncReadyCallback) _scanner_enumerator_next_cb, self);
}

static void
_scanner_enumerator_next_cb(GFileEnumerator *e, GAsyncResult *res, GelIOScanner *self)
{
	GelIOScannerPrivate *priv = self->priv;
	GError *error = NULL;
	GList *children = g_file_enumerator_next_files_finish(e, res, &error);
	GFile *parent   = g_file_enumerator_get_container(e);
	if (error)
	{
		g_file_enumerator_close(e, NULL, NULL);
		g_signal_emit(self, scanner_signals[ERROR], 0, e, error);
		g_error_free(error);
		g_object_unref(e);
		// _scanner_run_error(self, parent, error);
		_scanner_run_queue(self);
		return;
	}

	if (children == NULL)
	{
		g_file_enumerator_close(e, NULL, NULL);
		_scanner_run_queue(self);
		return;
	}

	GList *iter = children;
	while (iter)
	{
		GFileInfo *info = (GFileInfo *) iter->data;

		// Create GFile for each children
		GFile *file = g_file_get_child(parent, g_file_info_get_name(info));
		g_object_set_data((GObject *) file, "g-file-parent", parent);
		g_object_set_data((GObject *) file, "g-file-info",   info);

		// Add this to queue
		g_queue_push_tail(priv->queue, file);

		iter = iter->next;
	}

	g_list_free(children);
	g_file_enumerator_next_files_async(e, 8, G_PRIORITY_DEFAULT,
		priv->cancellable, (GAsyncReadyCallback) _scanner_enumerator_next_cb, self);
}


static gboolean
_scanner_run_queue_idle_wrapper(gpointer self)
{
	_scanner_run_queue(GEL_IO_SCANNER(self));
	return FALSE;
}

static void
_scanner_run_queue(GelIOScanner *self)
{
	GelIOScannerPrivate *priv = self->priv;
	if (g_queue_is_empty(priv->queue))
	{
		GList *l = priv->results = g_list_reverse(g_list_sort(priv->results, (GCompareFunc) _scanner_cmp_by_type_by_name_cb));
		while (l)
		{
			GNode *root = (GNode *) l->data;
			_scanner_sort_children(root);
	
			l = l->next;
		}

		g_signal_emit(self, scanner_signals[FINISH], 0, priv->results);
		return;
	}
	g_cancellable_reset(priv->cancellable);

	GFile     *file = g_queue_pop_head(priv->queue);
	GFileInfo *info = g_object_get_data((GObject *) file, "g-file-info");

	if (info == NULL)
	{
		// GFileInfo is needed
		g_file_query_info_async(file, priv->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
			priv->cancellable, (GAsyncReadyCallback) _scanner_query_info_cb, self);
	}
	else
	{
		GFile *parent = g_object_get_data((GObject *) file, "g-file-parent");
		GNode *pnode  = g_object_get_data((GObject *) parent, "x-node");
		if ((parent == NULL) || (pnode == NULL))
		{
			if (parent == NULL)
				g_warning("GFile has no parent");
			if (pnode == NULL)
				g_warning("Parent has no GNode associated");
			g_object_unref((GObject *) file);
		}
		else
		{
			GNode *node = g_node_new(file);
			g_object_set_data((GObject *) file, "x-node", node);
			g_node_append(pnode, node);
		}

		if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
			g_file_enumerate_children_async(file, priv->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
				priv->cancellable, (GAsyncReadyCallback) _scanner_enumerate_children_cb, self);
		else if (g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR)
			_scanner_run_queue(self);
		else 
		{
			g_warning("Unknow file type");
			_scanner_run_queue(self);
		}
	}
}

static gboolean
_scanner_free_node(GNode *node, gpointer data)
{
	GFileInfo *file_info = g_object_get_data((GObject *) node->data, "g-file-info");
	if (file_info)
		g_object_unref(file_info);
	g_object_unref(G_OBJECT(node->data));

	return FALSE;
}

static void
_scanner_sort_children(GNode *parent)
{
	GList *l = NULL;
	GNode *iter = parent->children;
	if (iter == NULL)
		return;

	// Unlink each child
	while (iter)
	{
		GNode *next = iter->next;
		g_node_unlink((GNode *) iter);
		l = g_list_prepend(l, iter);
		iter = next;
	}

	l = g_list_sort(l, (GCompareFunc) _scanner_cmp_by_type_by_name_cb);
	GList *i = l;

	while (i)
	{
		g_node_prepend(parent, (GNode *) i->data);
		if (((GNode *) i->data)->children)
			_scanner_sort_children((GNode *) i->data);
		i = i->next;
	}
	g_list_free(l);

	GNode *t = parent->children;
	while (t)
	{
		t = t->next;
	}
}


static gint
_scanner_cmp_by_type_by_name_cb(GNode *a, GNode *b)
{
	GFileInfo *_a, *_b;
	_a = g_object_get_data((GObject *) a->data, "g-file-info");
	_b = g_object_get_data((GObject *) b->data, "g-file-info");
	g_return_val_if_fail(_a && _b, 0);

	GFileType _ta = g_file_info_get_file_type(_a);
	GFileType _tb = g_file_info_get_file_type(_b);

	if (_ta != _tb)
	{
		if (_ta == G_FILE_TYPE_DIRECTORY)
			return -1;
		else
			return 1;
	}
	else
		return strcmp(g_file_info_get_name(_b), g_file_info_get_name(_a));
}

static gboolean
_scanner_traverse_cb(GNode *node, GList **list)
{
	*list = g_list_prepend(*list, node->data);
	return FALSE;
}

