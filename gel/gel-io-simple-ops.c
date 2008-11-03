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

struct _GelIOSimpleDir {
	GCancellable *cancellable;
	GList        *children;

	GelIOSimpleDirSuccessFunc   success;
	GelIOSimpleDirErrorFunc     error;
	GelIOSimpleDirCancelledFunc cancelled;
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

GelIOSimpleDir*
gel_io_simple_dir_read_full(GFile *file, const gchar *attributes,
	GelIOSimpleDirSuccessFunc   success,
	GelIOSimpleDirErrorFunc     error,
	GelIOSimpleDirCancelledFunc cancelled,
	gpointer data,
	GFreeFunc free_func)
{
	GelIOSimpleDir *self = g_new0(GelIOSimpleDir, 1);
	self->success   = success;
	self->error     = error;
	self->cancelled = cancelled;

	self->cancellable = g_cancellable_new();

	self->data = data;
	self->free_func = free_func;

    g_file_enumerate_children_async(file,
		attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		self->cancellable,
		enumerate_cb,
		self);

	return self;
}

void
gel_io_simple_dir_cancel(GelIOSimpleDir *self)
{
	g_cancellable_cancel(self->cancellable);

	g_object_unref(self->cancellable);
	gel_glist_free(self->children, (GFunc) g_object_unref, NULL);
}

void
gel_io_simple_dir_close(GelIOSimpleDir *self)
{
}

void
gel_io_simple_dir_set_data (GelIOSimpleDir *self, gpointer data)
{
	if (self->data && self->free_func)
		self->free_func(self->data);
	self->data = data;
}

gpointer
gel_io_simple_dir_get_data (GelIOSimpleDir *self)
{
	return self->data;
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
			self->error(self, err, self->data);
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
			self->error(self, err, self->data);
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
			self->error(self, err, self->data);
		g_error_free(err);
	}
	else
	{
		if (self->success)
			self->success(self, self->children, self->data);
	}
}

typedef struct GelIOMiniPack {
	const gchar *attributes;
	GelIOSimpleDirSuccessFunc   success;
	GelIOSimpleDirErrorFunc     error;
	GelIOSimpleDirCancelledFunc cancelled;
	GList *results;
	gpointer data;
	GFreeFunc free_func;
} GelIOMiniPack;

static void
gel_io_mini_pack_free(GelIOMiniPack *pack)
{
	gel_glist_free(pack->results, (GFunc) g_object_unref, NULL);
	g_free(pack);
}

static void
recursive_success(GelIOSimpleDir *op, GList *results, gpointer data);
static void
recursive_error(GelIOSimpleDir *op, GError *error, gpointer data);
static void
recursive_cancelled(GelIOSimpleDir *op, gpointer data);
static void
recursive_free_func(gpointer data);

GelIOSimpleDir* gel_io_simple_dir_read_recursive_full(GFile *file, const gchar *attributes,
	GelIOSimpleDirSuccessFunc   success,
	GelIOSimpleDirErrorFunc     error,
	GelIOSimpleDirCancelledFunc cancelled,
	gpointer  data,
	GFreeFunc free_func)
{
	GelIOMiniPack *pack = g_new0(GelIOMiniPack, 1);
	return gel_io_simple_dir_read_full(file, attributes,
		recursive_success,
		recursive_error,
		recursive_cancelled,
		pack,
		recursive_free_func);
}

static void
recursive_success(GelIOSimpleDir *op, GList *results, gpointer data)
{
	GList *children = results;
	while (children)
	{
		gel_warn("Got: %p %s", children->data, G_OBJECT_CLASS_NAME(G_OBJECT(children->data)));
		children = children->next;
	}
}

static void
recursive_error(GelIOSimpleDir *op, GError *error, gpointer data)
{
	GelIOMiniPack *pack = (GelIOMiniPack *) data;
	gel_glist_free(pack->results, (GFunc) g_object_unref, NULL);
	pack->error(op, error, pack->data);
	g_free(pack);
}

static void
recursive_cancelled(GelIOSimpleDir *op, gpointer data)
{
	GelIOMiniPack *pack = (GelIOMiniPack *) data;
	gel_glist_free(pack->results, (GFunc) g_object_unref, NULL);
	pack->cancelled(op, pack->data);
	g_free(pack);
}

static void
recursive_free_func(gpointer data)
{
	gel_io_mini_pack_free((GelIOMiniPack *) data);
}

