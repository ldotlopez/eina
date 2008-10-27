#ifndef GEL_IO_DISABLE_DEPRECATED

#define GEL_DOMAIN "Gel::IO:AsyncRead"
#include <gel/gel.h>
#include "gel-io-async-read-dir.h"

G_DEFINE_TYPE (GelIOAsyncReadDir, gel_io_async_read_dir, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_IO_TYPE_ASYNC_READ_DIR, GelIOAsyncReadDirPrivate))

typedef struct _GelIOAsyncReadDirPrivate GelIOAsyncReadDirPrivate;

struct _GelIOAsyncReadDirPrivate {
	gboolean running;
	GCancellable *cancellable;
	GList *children;
};

typedef enum {
	CANCEL,
	ERROR,
	FINISH,

	LAST_SIGNAL
} GelIOAsyncReadDirSignalType;
static guint gel_io_async_read_dir_signals[LAST_SIGNAL] = { 0 };

static void
_gel_io_async_read_dir_enumerate_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
_gel_io_async_read_dir_next_files_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
_gel_io_async_read_dir_close_cb(GObject *source, GAsyncResult *res, gpointer data);

static void
gel_io_async_read_dir_dispose (GObject *object)
{
	GelIOAsyncReadDir *self = GEL_IO_ASYNC_READ_DIR(object);
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);

	gel_io_async_read_dir_cancel(self);
	gel_glist_free(priv->children, (GFunc) g_object_unref, NULL);
	g_object_unref(priv->cancellable);

	if (G_OBJECT_CLASS (gel_io_async_read_dir_parent_class)->dispose)
		G_OBJECT_CLASS (gel_io_async_read_dir_parent_class)->dispose (object);
}

static void
gel_io_async_read_dir_class_init (GelIOAsyncReadDirClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelIOAsyncReadDirPrivate));

	object_class->dispose = gel_io_async_read_dir_dispose;

	gel_io_async_read_dir_signals[CANCEL] = g_signal_new ("cancel",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GelIOAsyncReadDirClass, cancel),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,
		0);
	gel_io_async_read_dir_signals[ERROR] = g_signal_new ("error",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GelIOAsyncReadDirClass, error),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
	gel_io_async_read_dir_signals[FINISH] = g_signal_new ("finish",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GelIOAsyncReadDirClass, finish),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
}

static void
gel_io_async_read_dir_init (GelIOAsyncReadDir *self)
{
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);
	priv->running     = FALSE;
	priv->cancellable = g_cancellable_new();
}

GelIOAsyncReadDir*
gel_io_async_read_dir_new (void)
{
	return g_object_new (GEL_IO_TYPE_ASYNC_READ_DIR, NULL);
}

void
gel_io_async_read_dir_scan(GelIOAsyncReadDir *self, GFile *file, const gchar *attributes)
{
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);

	if (priv->running)
		gel_io_async_read_dir_cancel(self);
	
	priv->running = TRUE;
	g_file_enumerate_children_async(file,
		attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		priv->cancellable,
		_gel_io_async_read_dir_enumerate_cb,
		self);
}

void
gel_io_async_read_dir_cancel(GelIOAsyncReadDir *self)
{
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);
	if (priv->running && !g_cancellable_is_cancelled(priv->cancellable))
		g_cancellable_cancel(priv->cancellable);

	g_cancellable_reset(priv->cancellable);
	gel_glist_free(priv->children, (GFunc) g_object_unref, NULL);
	priv->children = NULL;
	priv->running = TRUE;
}

static void
_gel_io_async_read_dir_enumerate_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadDir *self = GEL_IO_ASYNC_READ_DIR(data);
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);

	GFile           *file;
	GFileEnumerator *enumerator;
	GError          *err = NULL;

	file = G_FILE(source);
	enumerator = g_file_enumerate_children_finish(file, res, &err);
	if (enumerator == NULL)
	{
		priv->running = FALSE;
		g_signal_emit(G_OBJECT(self), gel_io_async_read_dir_signals[ERROR], 0, err);
		g_error_free(err);
		return;
	}

	g_file_enumerator_next_files_async(enumerator,
		4, G_PRIORITY_DEFAULT,
		priv->cancellable,
		_gel_io_async_read_dir_next_files_cb,
		(gpointer) self);
}

static void
_gel_io_async_read_dir_next_files_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadDir *self = GEL_IO_ASYNC_READ_DIR(data);
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);

	GFileEnumerator *enumerator;
	GList           *items;
	GError          *err = NULL;

	enumerator = G_FILE_ENUMERATOR(source);
	items = g_file_enumerator_next_files_finish(enumerator, res, &err);

	if (err != NULL)
	{
		priv->running = FALSE;
		g_object_unref(enumerator);
		gel_glist_free(items, (GFunc) g_object_unref, NULL);
		g_signal_emit(G_OBJECT(self), gel_io_async_read_dir_signals[ERROR], 0, err);
		g_error_free(err);
		return;
	}

	if (g_list_length(items) > 0)
	{
		priv->children = g_list_concat(priv->children, items);
		// g_list_free(items);

		g_file_enumerator_next_files_async(enumerator,
			4, G_PRIORITY_DEFAULT,
			priv->cancellable,
			_gel_io_async_read_dir_next_files_cb,
			(gpointer) data);
	}
	else
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_dir_signals[FINISH], 0, priv->children);
		g_file_enumerator_close_async(enumerator,
			G_PRIORITY_DEFAULT,
			priv->cancellable,
			_gel_io_async_read_dir_close_cb,
			(gpointer) data);
	}
}

static void
_gel_io_async_read_dir_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadDir *self = GEL_IO_ASYNC_READ_DIR(data);
	GelIOAsyncReadDirPrivate *priv = GET_PRIVATE(self);

	GFileEnumerator *enumerator;
	gboolean         result;
	GError          *err = NULL;

	enumerator = G_FILE_ENUMERATOR(source);
	result = g_file_enumerator_close_finish(enumerator, res, &err);

	priv->running = FALSE;
	g_object_unref(enumerator);

	if (result == FALSE)
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_dir_signals[ERROR], 0, err);
		g_error_free(err);
	}
}

#endif
