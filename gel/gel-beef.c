#include <gel/gel-beef.h>
#include <glib/gprintf.h>

struct _GelBeefOp {
	GCancellable *cancellable;
	GQueue *queue;

	GelBeefSuccessFunc success_cb;
	GelBeefErrorFunc   error_cb;
	GNode *result;

	GFile *root;
	GFile *parent;

	const gchar *attributes;

	gpointer data;

};

static void
_gel_beef_run_queue(GelBeefOp *self);
static void
_gel_beef_run_success(GelBeefOp *self);
static void
_gel_beef_run_error(GelBeefOp *self, GFile *source, GError *error);

static void
_gel_beef_assimile(GelBeefOp *self, GFile *parent, GFileInfo *info)
{
	GFile *f_child = g_file_get_child(parent, g_file_info_get_name(info));

	GNode *n_parent = (GNode *) g_object_get_data((GObject *) parent, "gnode");
	g_return_if_fail(n_parent != NULL);

	GNode *n_child = g_node_new(f_child);
	n_child->parent = n_parent;

	g_printf("Parent for %p -> %p\n", n_child, n_parent);
	g_object_set_data((GObject *) f_child, "gnode", n_child);

	g_queue_push_tail(self->queue, f_child);
}

static void
enumerator_cb(GFileEnumerator *e, GAsyncResult *res, GelBeefOp *self)
{
	GError *error = NULL;
	GFile *parent = g_file_enumerator_get_container(e);
	GList *children = g_file_enumerator_next_files_finish(e, res, &error);
	if (error)
	{
		_gel_beef_run_error(self, parent, error);
		return;
	}
	if (children == NULL)
	{
		g_file_enumerator_close(e, NULL, NULL);
		_gel_beef_run_queue(self);
		return;
	}

	GList *iter = children;
	while (iter)
	{
		_gel_beef_assimile(self, parent, (GFileInfo *) iter->data);
		iter = iter->next;
	}
	g_list_foreach(children, (GFunc) g_object_unref, NULL);
	g_list_free(children);
	g_file_enumerator_next_files_async(e, 8, G_PRIORITY_DEFAULT, self->cancellable, (GAsyncReadyCallback) enumerator_cb, self);
}

static void
enumerate_children_cb(GFile *source, GAsyncResult *res, GelBeefOp *self)
{
	GError *error = NULL;
	GFileEnumerator *e = g_file_enumerate_children_finish(source, res, &error);
	if (e == NULL)
	{
		_gel_beef_run_error(self, source, error);
		return;
	}

	g_file_enumerator_next_files_async(e, 8, G_PRIORITY_DEFAULT, self->cancellable, (GAsyncReadyCallback) enumerator_cb, self);
}

static void
query_info_cb(GFile *source, GAsyncResult *res, GelBeefOp *self)
{
	GError *error = NULL;
	GFileInfo *info = g_file_query_info_finish((GFile *) source, res, &error);
	if (info == NULL)
	{
		_gel_beef_run_error(self, source, error);
		return;
	}

	GFileType type = g_file_info_get_file_type(info);
	if (type == G_FILE_TYPE_DIRECTORY)
	{
		self->parent = source;
		g_printf("Examine children of %p\n", source);
		g_file_enumerate_children_async(source, self->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
			self->cancellable, (GAsyncReadyCallback) enumerate_children_cb, self);
	}
	else if (type == G_FILE_TYPE_REGULAR)
	{
		g_printf("Nothing to do, %p is regular\n", source);
		_gel_beef_run_queue(self);
	}
	else
	{
		g_printf("Unknow file type\n");
		_gel_beef_run_queue(self);
	}
}

GelBeefOp *
gel_beef_list(GFile *file, const gchar *attributes, gboolean recurse, 
  GelBeefSuccessFunc success_cb, GelBeefErrorFunc error_cb, gpointer data)
{
	GelBeefOp *self = g_new0(GelBeefOp, 1);
	self->cancellable = g_cancellable_new();
	self->queue = g_queue_new();
	self->attributes = attributes;
	self->success_cb  = success_cb;
	self->error_cb    = error_cb;

	self->result = g_node_new(file);
	self->root   = file;
	self->parent = file;


	self->data = data;

	// Insert into our tree-queue
	g_object_set_data((GObject *) file, "gnode",  self->result);
	g_queue_push_tail(self->queue, file);

	g_object_ref(file);

	_gel_beef_run_queue(self);

	return self;
}

void
gel_beef_cancel(GelBeefOp *self)
{
	if (self->cancellable)
	{
		g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->cancellable = NULL;
	}
}

void
gel_beef_free(GelBeefOp *self)
{
	g_queue_foreach(self->queue, (GFunc) g_object_unref, NULL);
	g_queue_free(self->queue);
	gel_beef_cancel(self);
	g_free(self);
}

static void
_gel_beef_run_queue(GelBeefOp *self)
{
	if (g_queue_is_empty(self->queue))
	{
		_gel_beef_run_success(self);
		return;
	}

	GFile *file = g_queue_pop_head(self->queue);
	g_cancellable_reset(self->cancellable);
	g_file_query_info_async(file, self->attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		self->cancellable,
		(GAsyncReadyCallback) query_info_cb, self);
}

static void
_gel_beef_run_success(GelBeefOp *self)
{
	if (self->success_cb)
		self->success_cb(self, self->root, self->result, self->data);
	gel_beef_free(self);
}

static void
_gel_beef_run_error(GelBeefOp *self, GFile *source, GError *error)
{
	if (self->error_cb)
		self->error_cb(self, source, error, self->data);
	g_object_unref(source);
	g_error_free(error);
	_gel_beef_run_queue(self);
}
