#ifndef GEL_IO_DISABLE_DEPRECATED

#define GEL_DOMAIN "Gel:IO::AsyncReadOp"
#include <gio/gio.h>
#include <gel/gel.h>
#include "gel-io-async-read-op.h"

G_DEFINE_TYPE (GelIOAsyncReadOp, gel_io_async_read_op, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_IO_TYPE_ASYNC_READ_OP, GelIOAsyncReadOpPrivate))

typedef struct _GelIOAsyncReadOpPrivate GelIOAsyncReadOpPrivate;

struct _GelIOAsyncReadOpPrivate {
	GByteArray   *ba;
	void         *buffer;
	GCancellable *cancellable;
	gboolean      running;
};

typedef enum {
	ERROR,
	FINISH,

	LAST_SIGNAL
} GelIOAsyncReadOpSignalType;
static guint gel_io_async_read_op_signals[LAST_SIGNAL] = { 0 };

static gboolean
gel_io_async_read_op_reset(GelIOAsyncReadOp *self);

void
_gel_io_async_read_op_open_cb(GObject *source, GAsyncResult *res, gpointer data);
void
_gel_io_async_read_op_read_cb(GObject *source, GAsyncResult *res, gpointer data);
void
_gel_io_async_read_op_close_cb(GObject *source, GAsyncResult *res, gpointer data);

static void
gel_io_async_read_op_dispose (GObject *object)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) object;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	if (gel_io_async_read_op_is_running(self))
		gel_io_async_read_op_cancel(self);

	gel_free_and_invalidate(priv->cancellable, NULL, g_object_unref);
	gel_free_and_invalidate(priv->buffer, NULL, g_free);
	gel_free_and_invalidate_with_args(priv->ba, NULL, g_byte_array_free, TRUE);

	if (G_OBJECT_CLASS (gel_io_async_read_op_parent_class)->dispose)
		G_OBJECT_CLASS (gel_io_async_read_op_parent_class)->dispose(object);
}

static void
gel_io_async_read_op_class_init (GelIOAsyncReadOpClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelIOAsyncReadOpPrivate));

	object_class->dispose = gel_io_async_read_op_dispose;

	gel_io_async_read_op_signals[ERROR] = g_signal_new ("error",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GelIOAsyncReadOpClass, error),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
	gel_io_async_read_op_signals[FINISH] = g_signal_new ("finish",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GelIOAsyncReadOpClass, finish),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
}

static void
gel_io_async_read_op_init (GelIOAsyncReadOp *self)
{
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	priv->cancellable = g_cancellable_new();

	priv->ba     = g_byte_array_new();
	priv->buffer = NULL;
}

GelIOAsyncReadOp*
gel_io_async_read_op_new (void)
{
	return g_object_new (GEL_IO_TYPE_ASYNC_READ_OP, NULL);
}

gboolean
gel_io_async_read_op_fetch(GelIOAsyncReadOp *self, GFile *file)
{
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	if (gel_io_async_read_op_is_running(self))
	{
		// XXX: Emit cancel
		gel_io_async_read_op_cancel(self);
		gel_io_async_read_op_reset(self);
	}

	priv->running = TRUE;
	g_file_read_async(file, G_PRIORITY_DEFAULT, priv->cancellable,
		_gel_io_async_read_op_open_cb, self);
	return TRUE;
}

gboolean
gel_io_async_read_op_reset(GelIOAsyncReadOp *self)
{
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	if (gel_io_async_read_op_is_running(self))
		gel_io_async_read_op_cancel(self);

	priv->running = FALSE;
	gel_free_and_invalidate_with_args(priv->ba, NULL, g_byte_array_free, TRUE);
	gel_free_and_invalidate(priv->buffer, NULL, g_free);
	priv->ba = g_byte_array_new();

	return TRUE;
}

gboolean
gel_io_async_read_op_is_running(GelIOAsyncReadOp* self)
{
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);
	return priv->running;
}

gboolean
gel_io_async_read_op_cancel(GelIOAsyncReadOp* self)
{
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	// Bug detection
	if (!g_cancellable_is_cancelled(priv->cancellable) && !priv->running)
	{
		gel_error("Bug detected, please file a bug report");
		return FALSE;
	}

	if (priv->running && !g_cancellable_is_cancelled(priv->cancellable))
		g_cancellable_cancel(priv->cancellable);
	g_cancellable_reset(priv->cancellable);
	priv->running = FALSE;

	return TRUE;
}

void
_gel_io_async_read_op_open_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) data;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	GFile        *file   = NULL;
	GInputStream *stream = NULL;
	GError       *err    = NULL;

	file = G_FILE(source);
	stream = G_INPUT_STREAM(g_file_read_finish(file, res, &err));
	if (err != NULL)
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[ERROR], 0, err);
		// gel_error ("Cannot create stream for file %p: %s", err->message, file);
		g_error_free(err);
		priv->running = FALSE;
		return;
	}

	priv->ba      = g_byte_array_new();
	priv->buffer  = g_new0(guint8, 32*1024);  // 32kb
	g_input_stream_read_async(stream, priv->buffer, 32*1024,
		G_PRIORITY_DEFAULT, priv->cancellable, _gel_io_async_read_op_read_cb, self);
}

void
_gel_io_async_read_op_read_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) data;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	GInputStream *stream;
	gssize        readed;
	GError        *err = NULL;

	stream = G_INPUT_STREAM(source);
	readed = g_input_stream_read_finish(stream, res, &err);
	if (err != NULL)
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[ERROR], 0, err);
		// gel_error("Got error while reading: '%s'", err->message);
		g_error_free(err);
		priv->running = FALSE;
		return;
	}

	if (readed > 0)
	{
		g_byte_array_append(priv->ba, (const guint8*) priv->buffer, readed);
		g_input_stream_read_async(stream, priv->buffer, 32*1024,
			G_PRIORITY_DEFAULT, priv->cancellable, _gel_io_async_read_op_read_cb, self);
		return;
	}

	g_free(priv->buffer);
	g_input_stream_close_async(stream, G_PRIORITY_DEFAULT, priv->cancellable,
		_gel_io_async_read_op_close_cb, self);
}

void
_gel_io_async_read_op_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) data;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	GInputStream *stream;
	GError       *err = NULL;

	stream = G_INPUT_STREAM(source);
	if (!g_input_stream_close_finish(stream, res, &err))
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[ERROR], 0, err);
		// gel_error("Cannot close '%s'", err->message);
		g_error_free(err);
		priv->running = FALSE;
		return;
	}
	priv->running = FALSE;

	g_object_ref(G_OBJECT(self));
	g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[FINISH], 0, priv->ba);
	g_object_unref(G_OBJECT(self));
}

#endif
