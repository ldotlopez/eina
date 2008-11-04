#define GEL_DOMAIN "Gel::IO::SimpleOps"
#include <gel/gel.h>
#include <gel/gel-io.h>

struct _GelIOSimpleFile {
	GCancellable *cancellable;
	GByteArray   *ba;
	guint8       *buffer;

	GelIOSimpleFileSuccessFunc   success;
	GelIOSimpleFileErrorFunc     error;
	GelIOSimpleFileCancelledFunc cancelled;
	gpointer data;
	GFreeFunc free_func;
};

static void
open_async_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_async_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_close_cb(GObject *source, GAsyncResult *res, gpointer data);

static void
enumerate_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
next_files_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
close_cb(GObject *source, GAsyncResult *res, gpointer data);

GelIOSimpleFile*
gel_io_simple_file_read_full(GFile *file,
	GelIOSimpleFileSuccessFunc   success,
	GelIOSimpleFileErrorFunc     error,
	GelIOSimpleFileCancelledFunc cancelled,
	gpointer data,
	GFreeFunc free_func)
{
	GelIOSimpleFile *self = g_new0(GelIOSimpleFile, 1);
	self->success   = success;
	self->error     = error;
	self->cancelled = cancelled;

	self->data      = data;
	self->free_func = free_func;

	self->cancellable = g_cancellable_new();
	g_object_ref(G_OBJECT(file));
	g_file_read_async(file,	G_PRIORITY_DEFAULT,
		self->cancellable, open_async_cb,
		self);

	return self;
}
	
void
gel_io_simple_file_cancel(GelIOSimpleFile *self)
{
	if (self->cancellable && !g_cancellable_is_cancelled(self->cancellable))
	{
		g_cancellable_cancel(self->cancellable);
		g_object_unref(G_OBJECT(self->cancellable));
		self->cancellable = NULL;
		self->cancelled(self, self->data);
	}
}

void
gel_io_simple_file_free(GelIOSimpleFile *self)
{
	gel_io_simple_file_cancel(self);
	gel_free_and_invalidate(self->buffer, NULL, g_free);
	gel_free_and_invalidate_with_args(self->ba, NULL, g_byte_array_free, TRUE);
	self->free_func(self->data);
	g_free(self);
}

static void
open_async_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOSimpleFile *self = (GelIOSimpleFile *) data;

	GError *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(g_file_read_finish(G_FILE(source), res, &err));
	g_object_unref(source);

	if (err != NULL)
	{
		if (self->error)
			self->error(self, err, self->data);
		g_error_free(err);
		return;
	}

	self->buffer  = g_new0(guint8, 32*1024);  // 32kb
	g_input_stream_read_async(stream, self->buffer, 32*1024,
		G_PRIORITY_DEFAULT, self->cancellable,
	read_async_cb, self);
}

static void
read_async_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOSimpleFile *self = (GelIOSimpleFile *) data;

	GError *err = NULL;
	gssize        readed;
	GInputStream *stream = G_INPUT_STREAM(source);

	readed = g_input_stream_read_finish(stream, res, &err);
	if (err != NULL)
	{
		self->error(self, err, self->data);
		g_error_free(err);
		return;
	}

	if (readed > 0)
	{
		if (self->ba == NULL)
			self->ba = g_byte_array_new();
		g_byte_array_append(self->ba, self->buffer, readed);
		g_free(self->buffer);
		g_input_stream_read_async(stream, self->buffer, 32*1024,
			G_PRIORITY_DEFAULT, self->cancellable, read_async_cb, self);
		return;
	}

	g_free(self->buffer);
	g_input_stream_close_async(stream, G_PRIORITY_DEFAULT, self->cancellable,
		read_close_cb, self);
}

static void
read_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOSimpleFile *self = (GelIOSimpleFile *) data;

	GError       *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(source);

	if (!g_input_stream_close_finish(stream, res, &err))
	{
		self->error(self, err, self->data);
		g_error_free(err);
		return;
	}

	self->success(self, self->ba, data);
}

// --
// GelIOSimpleDir
// --

struct _GelIOSimpleDir {
	GCancellable *cancellable;
	GFile        *parent;
	GList        *children;

	GelIOSimpleDirSuccessFunc   success;
	GelIOSimpleDirErrorFunc     error;
	GelIOSimpleDirCancelledFunc cancelled;
	gpointer data;
};

GelIOSimpleDir*
gel_io_simple_dir_read(GFile *file, const gchar *attributes,
	GelIOSimpleDirSuccessFunc   success,
	GelIOSimpleDirErrorFunc     error,
	GelIOSimpleDirCancelledFunc cancelled,
	gpointer data)
{
	GelIOSimpleDir *self = g_new0(GelIOSimpleDir, 1);
	self->parent    = file;

	self->success   = success;
	self->error     = error;
	self->cancelled = cancelled;

	self->cancellable = g_cancellable_new();

	self->data = data;

	g_object_ref(self->parent);
    g_file_enumerate_children_async(file,
		attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		self->cancellable,
		enumerate_cb,
		self);

	return self;
}

void
gel_io_simple_dir_close(GelIOSimpleDir *self)
{
	if (!g_cancellable_is_cancelled(self->cancellable))
		g_cancellable_cancel(self->cancellable);
	g_object_unref(self->parent);
	g_object_unref(self->cancellable);
	g_free(self);
}

void
gel_io_simple_dir_cancel(GelIOSimpleDir *self)
{
	g_cancellable_cancel(self->cancellable);
	gel_glist_free(self->children, (GFunc) g_object_unref, NULL);
	if (self->cancelled)
		self->cancelled(self, self->parent, self->data);
}

static void
enumerate_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOSimpleDir *self = (GelIOSimpleDir *) data;

	GFile           *file;
	GFileEnumerator *enumerator;
	GError          *err = NULL;

	file = G_FILE(source);
	enumerator = g_file_enumerate_children_finish(file, res, &err);
	if (enumerator == NULL)
	{
		if (self->error)
			self->error(self, self->parent, err, self->data);
		g_error_free(err);
		return;
	}

	g_file_enumerator_next_files_async(enumerator,
		4, G_PRIORITY_DEFAULT,
		self->cancellable,
		next_files_cb,
		self);
}

static void
next_files_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOSimpleDir  *self = (GelIOSimpleDir *) data;
	GError          *err = NULL;
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source);
	GList           *items = g_file_enumerator_next_files_finish(enumerator, res, &err);

	if (err != NULL)
	{
		gel_glist_free(items, (GFunc) g_object_unref, NULL);
		if (self->error)
			self->error(self, self->parent, err, self->data);
		g_error_free(err);
		return;
	}

	if (g_list_length(items) > 0)
	{
		self->children = g_list_concat(self->children, items);

		g_file_enumerator_next_files_async(enumerator,
			4, G_PRIORITY_DEFAULT,
			self->cancellable,
			next_files_cb,
			(gpointer) data);
	}
	else
	{
		g_file_enumerator_close_async(enumerator,
			G_PRIORITY_DEFAULT,
			self->cancellable,
			close_cb,
			(gpointer) data);
	}
}

static void
close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOSimpleDir *self = (GelIOSimpleDir *) data;

	GError          *err = NULL;
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source);
	gboolean         result = g_file_enumerator_close_finish(enumerator, res, &err);

	if (result == FALSE)
	{
		if (self->error)
			self->error(self, self->parent, err, self->data);
		g_error_free(err);
	}
	else
	{
		if (self->success)
			self->success(self, self->parent, self->children, self->data);
	}
}

// --
// GelIOSimpleDirRecurse
// --
struct _GelIOSimpleDirRecurse {
	GFile *parent;
	GList *ops;
	gpointer user_data;
	GList *parents;
};

static void
recurse_read_success_cb(GelIOSimpleDir *op, GFile *parent, GList *children, gpointer data);
static void
recurse_read_error_cb(GelIOSimpleDir *op, GFile *parent, GError *error, gpointer data);
static void
recurse_read_cancelled_cb(GelIOSimpleDir *op, GFile *parent, gpointer data);

GelIOSimpleDirRecurse*
gel_io_simple_dir_recurse_read(GFile *file, const gchar *attributes,
	GelIOSimpleDirRecurseSuccessFunc   success,
	GelIOSimpleDirRecurseErrorFunc     error,
	GelIOSimpleDirRecurseCancelledFunc cancelled,
	gpointer  data)
{
	GelIOSimpleDirRecurse *self = g_new0(GelIOSimpleDirRecurse, 1);

	self->parent    = file;
	self->ops       = NULL;
	self->user_data = data;
	self->parents   = g_list_prepend(NULL, g_list_prepend(NULL, file)); // Yes!
	gel_warn("Root parent: %p == %p", self->parents, file);
	g_object_ref(self->parent);
	
	GelIOSimpleDir *sub_op = gel_io_simple_dir_read(self->parent, attributes,
		recurse_read_success_cb,
		recurse_read_error_cb,
		recurse_read_cancelled_cb,
		self);
	self->ops = g_list_prepend(self->ops, sub_op);

	return self;
}

void gel_io_simple_dir_recurse_close
(GelIOSimpleDirRecurse *op)
{
}

void gel_io_simple_dir_recurse_cancel
(GelIOSimpleDirRecurse *op)
{
}

void recurse_add_parent(GelIOSimpleDirRecurse *self, GFile *new_parent)
{
	self->parents = g_list_prepend(self->parents, g_list_prepend(NULL, new_parent));
}

GList *recurse_find_parent(GelIOSimpleDirRecurse *self, GFile *parent)
{
	GList *iter = self->parents;
	while (iter)
	{
		if (((GList *) iter->data)->data == parent)
		// if (g_list_find((GList *) iter->data, parent))
			break;
		iter = iter->next;
	}
	if (!iter)
		gel_warn("Parent not found");
	return iter;
}

void recurse_add_children(GelIOSimpleDirRecurse *self, GFile *parent, GList *children)
{
	GList *parent_list = recurse_find_parent(self, parent);
	GList *new_parent_list = g_list_concat(parent_list, g_list_copy(children));

	GList *iter = self->parents;
	while (iter)
	{
		if (iter->data == parent_list)
		{
			iter->data = new_parent_list;
			break;
		}
		iter = iter->next;
	}
	if (!iter)
		gel_warn("Cannot add children");
}

static GList *
recurse_find_in_parents(GelIOSimpleDirRecurse *self, GFile *file)
{
	GList *iter = self->parents;
	GList *ret = NULL;
	while (iter)
	{
		if ((ret = g_list_find((GList *) iter->data, file)) != NULL)
			break;
		iter = iter->next;
	}
	return iter;
}

static void
recurse_read_success_cb(GelIOSimpleDir *op, GFile *parent, GList *children, gpointer data)
{
	GelIOSimpleDirRecurse *self = (GelIOSimpleDirRecurse *) data;
	if (!recurse_find_in_parents(self, parent))
		recurse_add_parent(self, parent);
	recurse_add_children(self, parent, children);
/*
	GList *parent_list = recurse_find_in_parents(self, parent);
	gel_warn("GFile responsable: %p Parent list: %p (%p)", parent, parent_list, parent_list ? ((GList *) parent_list->data)->data : NULL);
	GList *root_list = g_list_find(self->parents, parent_list);
	root_list->data = g_list_concat((GList *) root_list->data, children);
*/
}

static void
recurse_read_error_cb(GelIOSimpleDir *op, GFile *parent, GError *error, gpointer data)
{
}

static void
recurse_read_cancelled_cb(GelIOSimpleDir *op, GFile *parent, gpointer data)
{
}
