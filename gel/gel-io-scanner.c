#include <gel/gel.h>
#include <gel/gel-io-scanner.h>
#include <string.h>

#define GEL_DOMAIN "Gel::IO::Scanner"

struct _GelIOScanner {
	GList                   *uris;
	const gchar             *attributes;
	gboolean                 recurse;
	GelIOScannerSuccessFunc  success_cb;
	GelIOScannerErrorFunc    error_cb;
	gpointer                 userdata;

	GList *results;
	GQueue *queue;
	GCancellable *cancellable;

	gboolean disposed;
};

static void
_scanner_run_success(GelIOScanner *self);
static void
_scanner_run_error(GelIOScanner *self, GFile *file, GError *error);
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

GelIOScanner*
gel_io_scan(GList *uris, const gchar *attributes, gboolean recurse,
    GelIOScannerSuccessFunc success_cb, GelIOScannerErrorFunc error_cb, gpointer data)
{
	GelIOScanner *self = g_new0(GelIOScanner, 1);
	self->attributes = attributes;
	self->recurse    = recurse;
	self->success_cb = success_cb;
	self->error_cb   = error_cb;
	self->userdata   = data;

	self->cancellable = g_cancellable_new();
	self->queue = g_queue_new();
	GList *iter = uris;
	while (iter)
	{
		GFile *file = g_file_new_for_uri(iter->data);
		g_queue_push_tail(self->queue, file);

		iter = iter->next;
	}

	_scanner_run_queue(self);

	return self;
}

void
gel_io_scan_close(GelIOScanner *self)
{
	if (self->disposed)
		return;

	if (self->cancellable)
	{
		g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->cancellable = NULL;
	}
	if (self->queue)
	{
		g_queue_foreach(self->queue, (GFunc) _scanner_free_node, NULL);
		g_queue_foreach(self->queue, (GFunc) g_node_destroy, NULL);
		g_queue_free(self->queue);
		self->queue = NULL;
	}
	if (self->results)
	{
		GList *iter = self->results;
		while (iter)
		{
			g_node_traverse((GNode *) iter->data, G_IN_ORDER, G_TRAVERSE_ALL, -1, _scanner_free_node, NULL);
			g_node_destroy((GNode *) iter->data);
			iter = iter->next;
		}
		g_list_free(self->results);
		self->results = NULL;
	}

	self->disposed = TRUE;
}

GList*
gel_io_scan_flatten_result(GList *forest)
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
_scanner_run_success(GelIOScanner *self)
{
	GList *l = self->results = g_list_reverse(g_list_sort(self->results, (GCompareFunc) _scanner_cmp_by_type_by_name_cb));
	while (l)
	{
		GNode *root = (GNode *) l->data;
		_scanner_sort_children(root);

		l = l->next;
	}

	if (self->success_cb)
		self->success_cb(self, self->results, self->userdata);
	gel_io_scan_close(self);
	g_free(self);
}

static void
_scanner_run_error(GelIOScanner *self, GFile *file, GError *error)
{
	if (self->error_cb)
		self->error_cb(self, file, error, self->userdata);

	g_object_unref(file);
	g_error_free(error);
}

static void
_scanner_query_info_cb(GFile *source, GAsyncResult *res, GelIOScanner *self)
{
	GError *error = NULL;
	GFileInfo *info = g_file_query_info_finish((GFile *) source, res, &error);
	if (info == NULL)
	{
		_scanner_run_error(self, source, error);
		_scanner_run_queue(self);
		return;
	}

	// This GFile is already completed, add to results (it is a root)
	GNode *node = g_node_new(source);
	g_object_set_data((GObject *) source, "x-node",      node);
	g_object_set_data((GObject *) source, "g-file-info", info);
	self->results = g_list_append(self->results, node);

	GFileType type = g_file_info_get_file_type(info);
	if ((type == G_FILE_TYPE_DIRECTORY) && self->recurse)
	{
		g_file_enumerate_children_async(source, self->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
			self->cancellable, (GAsyncReadyCallback) _scanner_enumerate_children_cb, self);
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
	GError *error = NULL;
	GFileEnumerator *e = g_file_enumerate_children_finish(source, res, &error);
	if (e == NULL)
	{
		_scanner_run_error(self, source, error);
		_scanner_run_queue(self);
		return;
	}

	g_file_enumerator_next_files_async(e, 8, G_PRIORITY_DEFAULT,
		self->cancellable, (GAsyncReadyCallback) _scanner_enumerator_next_cb, self);
}

static void
_scanner_enumerator_next_cb(GFileEnumerator *e, GAsyncResult *res, GelIOScanner *self)
{
	GError *error = NULL;
	GList *children = g_file_enumerator_next_files_finish(e, res, &error);
	GFile *parent   = g_file_enumerator_get_container(e);
	if (error)
	{
		g_file_enumerator_close(e, NULL, NULL);
		_scanner_run_error(self, parent, error);
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
		g_queue_push_tail(self->queue, file);

		iter = iter->next;
	}

	g_list_free(children);
	g_file_enumerator_next_files_async(e, 8, G_PRIORITY_DEFAULT,
		self->cancellable, (GAsyncReadyCallback) _scanner_enumerator_next_cb, self);
}


static void
_scanner_run_queue(GelIOScanner *self)
{
	if (g_queue_is_empty(self->queue))
	{
		_scanner_run_success(self);
		return;
	}
	g_cancellable_reset(self->cancellable);

	GFile     *file = g_queue_pop_head(self->queue);
	GFileInfo *info = g_object_get_data((GObject *) file, "g-file-info");

	if (info == NULL)
	{
		// GFileInfo is needed
		g_file_query_info_async(file, self->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
			self->cancellable, (GAsyncReadyCallback) _scanner_query_info_cb, self);
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
			g_file_enumerate_children_async(file, self->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
				self->cancellable, (GAsyncReadyCallback) _scanner_enumerate_children_cb, self);
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


