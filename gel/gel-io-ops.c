#define GEL_DOMAIN "Gel::IO::SimpleOps"
#include <gio/gio.h>
#include <gel/gel.h>
#include <gel/gel-io-misc.h>
#include <gel/gel-io-ops.h>
#include <gel/gel-io-op-result.h>

typedef void (*GelIOOpSubOpFreeResources) (GelIOOp *self);
struct _GelIOOp {
	guint               refs;
	GFile              *source;
	GCancellable       *cancellable;
	GelIOOpSuccessFunc  success;
	GelIOOpErrorFunc    error;
	gpointer            data;
	gpointer            _d;
	GelIOOpSubOpFreeResources resource_free;
};

static void
cancelled_cb(GCancellable *cancellable, GelIOOp *self);

GelIOOp *gel_io_op_new(GFile *source,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = g_new0(GelIOOp, 1);
	g_object_ref(source);

	self->refs   = 1;
	self->source = source;
	self->cancellable = g_cancellable_new();
	self->success = success;
	self->error = error;
	self->data = data;

	g_signal_connect(self->cancellable, "cancelled",
	G_CALLBACK(cancelled_cb), self);

	return self;
}

void
gel_io_op_ref(GelIOOp *self)
{
	self->refs++;
}

void
gel_io_op_unref(GelIOOp *self)
{
	self->refs--;
	if (self->refs)
		return;
	gel_io_op_destroy(self);
}

void
gel_io_op_destroy(GelIOOp *self)
{
	// Stop signals
	g_signal_handlers_disconnect_by_func(self->cancellable, cancelled_cb, self);

	// Cancel operations
	if (!g_cancellable_is_cancelled(self->cancellable))
		g_cancellable_cancel(self->cancellable);

	// Call subop freeder
	if (self->resource_free && (self->_d != NULL))
		gel_free_and_invalidate(self->_d, NULL, self->resource_free);
	
	// Free other resources and ourselves
	g_object_unref(self->cancellable);
	g_object_unref(self->source);
	g_free(self);
}

void
gel_io_op_cancel(GelIOOp *self)
{
	g_signal_handlers_disconnect_by_func(self->cancellable, cancelled_cb, self);
	if (!g_cancellable_is_cancelled(self->cancellable))
		g_cancellable_cancel(self->cancellable);
}

GFile*
gel_io_op_get_source(GelIOOp *self)
{
	return self->source;
}

void
gel_io_op_success(GelIOOp *self, GelIOOpResult *result)
{
	if (self->success == NULL)
		return;
	self->success(self, self->source, result, self->data);
}

void
gel_io_op_error(GelIOOp *self, GError *error)
{
	if (self->error)
		self->error(self, self->source, error, self->data);
	g_error_free(error);
}

static void
cancelled_cb(GCancellable *cancellable, GelIOOp *self)
{
	GError *error = NULL;
	if (!g_cancellable_is_cancelled(self->cancellable))
	{
		gel_warn("cancelled signal recived but cancellable is not cancelled");
		return;
	}

	if (self->error)
	{
		g_cancellable_set_error_if_cancelled(self->cancellable, &error);
		self->error(self, self->source, error, self->data);
		g_error_free(error);
	}
}

// --
// file_read operation
// --
#define FILE_READ(p) ((GelIOOpFileRead*)p)
typedef struct {
	guint8 *buffer;
	GByteArray *ba;
} GelIOOpFileRead;

static void
read_file_open_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_file_read_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_file_close_cb(GObject *source, GAsyncResult *res, gpointer data);

void
read_file_destroy(GelIOOp *self)
{
	GelIOOpFileRead *d = FILE_READ(self->_d);
	gel_free_and_invalidate(d->buffer, NULL, g_free);
	gel_free_and_invalidate_with_args(d->ba, NULL, g_byte_array_free, TRUE);
	g_free(d);
}

GelIOOp *gel_io_read_file(GFile *file,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(file, success, error, data);
	self->resource_free = read_file_destroy;
	self->_d = g_new0(GelIOOpFileRead, 1);
	g_file_read_async(file, G_PRIORITY_DEFAULT,
		self->cancellable, read_file_open_cb,
		self);
	return self;
}

static void
read_file_open_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(g_file_read_finish(G_FILE(source), res, &err));

	if (err != NULL)
	{
		gel_io_op_error(self, err);
		return;
	}

	FILE_READ(self->_d)->buffer = g_new0(guint8, 32*1024);  // 32kb
	g_input_stream_read_async(stream, FILE_READ(self->_d)->buffer, 32*1024,
		G_PRIORITY_DEFAULT, self->cancellable,
	read_file_read_cb, self);
}

static void
read_file_read_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError *err = NULL;
	gssize        readed;
	GInputStream *stream = G_INPUT_STREAM(source);

	readed = g_input_stream_read_finish(stream, res, &err);
	if (err != NULL)
	{
		gel_io_op_error(self, err);
		return;
	}

	if (readed > 0)
	{
		GelIOOpFileRead *fr = FILE_READ(self->_d);
		if (fr->ba == NULL)
			fr->ba = g_byte_array_new();

		g_byte_array_append(fr->ba, fr->buffer, readed);
		g_free(fr->buffer);
		g_input_stream_read_async(stream, fr->buffer, 32*1024,
			G_PRIORITY_DEFAULT, self->cancellable, read_file_read_cb, self);
		return;
	}

	g_free(FILE_READ(self->_d)->buffer);
	g_input_stream_close_async(stream, G_PRIORITY_DEFAULT, self->cancellable,
		read_file_close_cb, self);
}

static void
read_file_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError       *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(source);

	if (!g_input_stream_close_finish(stream, res, &err))
	{
		gel_io_op_error(self, err);
		return;
	}

	GelIOOpResult *result = gel_io_op_result_new(GEL_IO_OP_RESULT_BYTE_ARRAY, FILE_READ(self->_d)->ba);
	gel_io_op_success(self, result);
	gel_io_op_result_unref(result);
}

// --
// read_dir operation
// --

static void
read_dir_enumerate_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_dir_next_files_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_dir_close_cb(GObject *source, GAsyncResult *res, gpointer data);

void
read_dir_destroy(GelIOOp *self)
{
	gel_glist_free((GList *) self->_d, (GFunc) g_object_unref, NULL);
}

GelIOOp *gel_io_read_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(dir, success, error, data);
	self->resource_free = read_dir_destroy;
    g_file_enumerate_children_async(dir,
		attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		self->cancellable,
		read_dir_enumerate_cb,
		self);
	return self;
}

static void
read_dir_enumerate_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GFile           *dir;
	GFileEnumerator *enumerator;
	GError          *err = NULL;

	dir = G_FILE(source);
	enumerator = g_file_enumerate_children_finish(dir, res, &err);
	if (enumerator == NULL)
	{
		gel_io_op_error(self, err);
		return;
	}

	g_file_enumerator_next_files_async(enumerator,
		4, G_PRIORITY_DEFAULT,
		self->cancellable,
		read_dir_next_files_cb,
		self);
}

static void
read_dir_next_files_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);
	GError          *err = NULL;
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source);
	GList           *items = g_file_enumerator_next_files_finish(enumerator, res, &err);

	if (err != NULL)
	{
		gel_glist_free(items, (GFunc) g_object_unref, NULL);
		gel_io_op_error(self, err);
		return;
	}

	if (g_list_length(items) > 0)
	{
		self->_d = (gpointer) g_list_concat((GList *) self->_d, items);

		g_file_enumerator_next_files_async(enumerator,
			4, G_PRIORITY_DEFAULT,
			self->cancellable,
			read_dir_next_files_cb,
			data);
	}
	else
	{
		g_file_enumerator_close_async(enumerator,
			G_PRIORITY_DEFAULT,
			self->cancellable,
			read_dir_close_cb,
			data);
	}
}

static void
read_dir_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError          *err = NULL;
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source);
	gboolean         result = g_file_enumerator_close_finish(enumerator, res, &err);

	if (result == FALSE)
	{
		gel_io_op_error(self, err);
	}
	else
	{
		GelIOOpResult *r = gel_io_op_result_new(GEL_IO_OP_RESULT_OBJECT_LIST, self->_d);
		gel_io_op_success(self, r);
		gel_io_op_result_unref(r);
	}
}

// --
// recurse dir operation
// --

typedef struct {
	const gchar        *attributes;
	GelIOOpSuccessFunc  user_success;
	GelIOOpErrorFunc    user_error;
	GList              *ops;
	GelIORecurseTree   *tree;
} GelIOOpRecuseDir;
#define RECURSE_DIR(p) ((GelIOOpRecuseDir*)p)

void recurse_dir_destroy(GelIOOp *self)
{
	GelIOOpRecuseDir *d = RECURSE_DIR(self->_d);

	if (d->ops)
	{
		gel_glist_free(d->ops, (GFunc) gel_io_op_cancel, NULL);
		d->ops = NULL;
	}
	if (d->tree)
	{
		gel_io_recurse_tree_unref(d->tree);
		d->tree = NULL;
	}
}

static void
recurse_dir_success_cb(GelIOOp *op, GFile *parent, GelIOOpResult *result, gpointer data);
static void
recurse_dir_error_cb(GelIOOp *op, GFile *parent, GError *error, gpointer data);

GelIOOp*
gel_io_recurse_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(dir, success, error, data);
	self->resource_free = recurse_dir_destroy;
	self->_d = g_new0(GelIOOpRecuseDir, 1);
	RECURSE_DIR(self->_d)->tree = gel_io_recurse_tree_new();
	RECURSE_DIR(self->_d)->attributes = attributes;
	RECURSE_DIR(self->_d)->ops = g_list_prepend(NULL, gel_io_read_dir(dir, attributes,
		recurse_dir_success_cb,
		recurse_dir_error_cb,
		self));

	return self;
}

void
gel_io_recurse_dir_remove_op(GelIOOp *op, GelIOOp *subop)
{
	GelIOOpRecuseDir *r = RECURSE_DIR(op->_d);
	r->ops = g_list_remove(r->ops, subop);
	if (r->ops != NULL)
		return;

	GelIOOpResult *res = gel_io_op_result_new(GEL_IO_OP_RESULT_RECURSE_TREE, RECURSE_DIR(op->_d)->tree);
	gel_io_op_success(op, res);
	gel_io_op_result_unref(res);
}

static void
recurse_dir_success_cb(GelIOOp *op, GFile *parent, GelIOOpResult *result, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GList *children = gel_io_op_result_get_object_list(result);

	// Add to result
	gel_io_recurse_tree_add_parent(RECURSE_DIR(self->_d)->tree, parent);
	gel_io_recurse_tree_add_children(RECURSE_DIR(self->_d)->tree, parent, children);

	// Launch suboperations for all directories in children
	GList *iter = children;
	while (iter)
	{
		GFileInfo *child_info = G_FILE_INFO(iter->data);
		if (g_file_info_get_file_type(child_info) != G_FILE_TYPE_DIRECTORY)
		{
			iter = iter->next;
			continue;
		}

		GFile *child = gel_io_file_get_child_for_file_info(parent, child_info);
		RECURSE_DIR(self->_d)->ops = g_list_prepend(RECURSE_DIR(self->_d)->ops,
			gel_io_read_dir(child, RECURSE_DIR(self->_d)->attributes,
				recurse_dir_success_cb,
				recurse_dir_error_cb,
				self
				)
			);
		iter = iter->next;
	}

	// Remove operation from queue
	gel_io_recurse_dir_remove_op(self, op);
}

static void
recurse_dir_error_cb(GelIOOp *op, GFile *parent, GError *error, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	if (self->error)
		self->error(self, parent, error, self->data);
	/*
	gchar *uri = g_file_get_uri(parent);
	gel_warn("Call %p, Got error for '%s': %s'",
		RECURSE_DIR(self->_d)->user_error, uri, error->message);
	g_free(uri);
	*/
}
