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
#include <glib/gprintf.h>
struct _GelIOSimpleDirRecurse {
	GFile *parent;
	const gchar *attributes;
	GList *parents;
	GList *ops;

	// User data
	GelIOSimpleDirRecurseSuccessFunc   success;
	GelIOSimpleDirRecurseErrorFunc     error;
	GelIOSimpleDirRecurseCancelledFunc cancelled;
	gpointer data;
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
	g_object_ref(file); // Hold it

	self->parent     = file;
	self->attributes = attributes;
	self->parents    = g_list_prepend(NULL, g_list_prepend(NULL, file)); // Yes!

	// Save user data
	self->success   = success;
	self->error     = error;
	self->cancelled = cancelled;
	self->data      = data;

	// Start recursion
	self->ops = g_list_prepend(NULL, gel_io_simple_dir_read(self->parent, attributes,
		recurse_read_success_cb,
		recurse_read_error_cb,
		recurse_read_cancelled_cb,
		self));

	return self;
}

void gel_io_simple_dir_recurse_close
(GelIOSimpleDirRecurse *self)
{
	// Cancel any pending operation
	gel_io_simple_dir_recurse_cancel(self);

	// XXX: Delete children
	// gel_io_simple_dir_recurse_result_free((GelIOSimpleDirRecurseResult *) self->parents);

	g_object_unref(self->parent);

	g_free(self);
}

void gel_io_simple_dir_recurse_cancel
(GelIOSimpleDirRecurse *self)
{
	GList *op_iter = self->ops;
	while (op_iter)
	{
		gel_io_simple_dir_cancel((GelIOSimpleDir *) op_iter->data);
		gel_io_simple_dir_close((GelIOSimpleDir *) op_iter->data);
		op_iter = op_iter->next;
	}
	g_list_free(self->ops);
	self->ops = NULL;
}

void recurse_add_parent(GelIOSimpleDirRecurse *self, GFile *new_parent)
{
	self->parents = g_list_append(self->parents, g_list_prepend(NULL, new_parent));
}

GList *recurse_find_in_parents(GelIOSimpleDirRecurse *self, GFile *parent)
{
	GList *iter = self->parents;
	while (iter)
	{
		if (((GList *) iter->data)->data == parent)
			break;
		iter = iter->next;
	}
	if (!iter)
		gel_warn("Parent not found");
	return iter;
}

void recurse_add_children(GelIOSimpleDirRecurse *self, GFile *parent, GList *children)
{
	GList *parent_list = recurse_find_in_parents(self, parent);
	if (parent_list == NULL)
	{
		//gel_error("Unable to find %p in parents list", parent);
		return;
	}

	parent_list->data = g_list_concat((GList *) parent_list->data, g_list_copy(children));
	// g_printf("Added %d children for %s\n", g_list_length(children), g_file_get_uri(parent));
}

void recurse_print(GelIOSimpleDirRecurse *self)
{
	GList *iter, *iter2;
	iter = self->parents;
	while (iter)
	{
		iter2 = iter->data;
		while (iter2)
		{
			g_printf("%p ", iter2->data);
			iter2 = iter2->next;
		}
		g_printf("\n");
		iter = iter->next;
	}
}

static void
recurse_remove_operation(GelIOSimpleDirRecurse *self, GelIOSimpleDir *op)
{
	if (g_list_find(self->ops, op) == NULL)
	{
		gel_error("Attempt to remove an invalid operation!");
		return;
	}

	// Call to success if there are no more pending operations
	if (((self->ops = g_list_remove(self->ops, op)) == NULL) && self->success)
		self->success(self, self->parent, (GelIOSimpleDirRecurseResult*) self->parents, self->data);
}

static void
recurse_read_success_cb(GelIOSimpleDir *op, GFile *parent, GList *children, gpointer data)
{
	GelIOSimpleDirRecurse *self = (GelIOSimpleDirRecurse *) data;
	GList *iter = children;

	if (!recurse_find_in_parents(self, parent))
		recurse_add_parent(self, parent);
	recurse_add_children(self, parent, children);

	while (iter)
	{
		GFileInfo *child_info = G_FILE_INFO(iter->data);
		if (g_file_info_get_file_type(child_info) == G_FILE_TYPE_DIRECTORY)
		{
			GFile *child = gel_io_file_get_child_for_file_info(parent, child_info);
			recurse_add_parent(self, child);
			GelIOSimpleDir *sub_op = gel_io_simple_dir_read(child, self->attributes,
				recurse_read_success_cb,
				recurse_read_error_cb,
				recurse_read_cancelled_cb,
				self);
			self->ops = g_list_prepend(self->ops, sub_op);
		}
		iter = iter->next;
	}

	// Remove operation
	recurse_remove_operation(self, op);
}

static void
recurse_read_error_cb(GelIOSimpleDir *op, GFile *parent, GError *error, gpointer data)
{
	GelIOSimpleDirRecurse *self = (GelIOSimpleDirRecurse *) data;

	// Call user data
	if (self->error)
		self->error(self, parent, error, self->data);

	// Remove operation
	recurse_remove_operation(self, op);
}

static void
recurse_read_cancelled_cb(GelIOSimpleDir *op, GFile *parent, gpointer data)
{
	GelIOSimpleDirRecurse *self = (GelIOSimpleDirRecurse *) data;

	// Remove operation
	recurse_remove_operation(self, op);
}

// --
// Recursive result
// --
typedef GList _GelIOSimpleDirRecurseResult;

GFile *
gel_io_simple_dir_recurse_result_get_root            (GelIOSimpleDirRecurseResult *res)
{
	return G_FILE(((GList*) ((GList*)res)->data)->data);
}

GList *
gel_io_simple_dir_recurse_result_get_children        (GelIOSimpleDirRecurseResult *res, GFile *node)
{
	GList *iter = (GList *) res;
	GList *ret = NULL;

	while (iter)
	{
		if (g_file_equal(G_FILE(((GList *) iter->data)->data), node))
		{
			ret = g_list_copy(iter->data);
			ret = g_list_remove(ret, ret->data);
			return ret;
		}
		iter = iter->next;
	}
	return NULL;
}

GList *gel_io_simple_dir_recurse_result_get_children_as_file(GelIOSimpleDirRecurseResult *res, GFile *node);
void   gel_io_simple_dir_recurse_result_free                (GelIOSimpleDirRecurseResult *res);
