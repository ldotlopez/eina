#define GEL_DOMAIN "Gel:IO::AsyncReadOp"
#include <gio/gio.h>
#include <gel/gel.h>
#include "gel-io-async-read-op.h"

G_DEFINE_TYPE (GelIOAsyncReadOp, gel_io_async_read_op, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_IO_TYPE_ASYNC_READ_OP, GelIOAsyncReadOpPrivate))

typedef struct _GelIOAsyncReadOpPrivate GelIOAsyncReadOpPrivate;

struct _GelIOAsyncReadOpPrivate {
	GFile        *file;
	GInputStream *stream;
	GByteArray   *ba;
	void         *buffer;
	GCancellable *cancellable;
	gboolean      callback_in_progress;
	GelIOAsyncReadOpPhase phase;
};

typedef enum {
	ERROR,
	FINISH,

	LAST_SIGNAL
} GelIOAsyncReadOpSignalType;
static guint gel_io_async_read_op_signals[LAST_SIGNAL] = { 0 };

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

	if (priv->callback_in_progress)
		gel_warn("unref while callback, rejecting");
	else
		gel_info("No callback in progress, disposing");

	if ((priv->phase != GEL_IO_ASYNC_READ_OP_PHASE_NONE) && (priv->phase != GEL_IO_ASYNC_READ_OP_PHASE_FINISH))
	{
		gel_warn("Disposing %p. Operation in progress will be cancelled");
		g_cancellable_cancel(priv->cancellable);
	}

	gel_free_and_invalidate(priv->file, NULL, g_object_unref);
	gel_free_and_invalidate(priv->cancellable, NULL, g_object_unref);
	gel_free_and_invalidate(priv->stream, NULL, g_object_unref);
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

	priv->file = NULL;
	priv->stream = NULL;
	priv->cancellable = g_cancellable_new();
	priv->buffer = NULL;
	priv->ba = NULL;
	priv->phase = GEL_IO_ASYNC_READ_OP_PHASE_NONE;
	priv->callback_in_progress = FALSE;
}

GelIOAsyncReadOp*
gel_io_async_read_op_new (gchar *uri)
{
	GelIOAsyncReadOp* self = g_object_new (GEL_IO_TYPE_ASYNC_READ_OP, NULL);
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);

	priv->file = g_file_new_for_uri(uri);
	g_file_read_async(priv->file, G_PRIORITY_DEFAULT, priv->cancellable,
		_gel_io_async_read_op_open_cb, self);

	return self;
}

gboolean
gel_io_async_read_op_cancel(GelIOAsyncReadOp* self)
{
	return FALSE;
}

void
_gel_io_async_read_op_open_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) data;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);
	GError *err = NULL;

	priv->stream = G_INPUT_STREAM(g_file_read_finish(priv->file, res, &err));
	if (err != NULL)
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[ERROR], 0, err);
		gel_error ("Cannot create stream: %s", err->message);
		g_error_free(err);
		priv->phase = GEL_IO_ASYNC_READ_OP_PHASE_ERROR;
		return;
	}

	priv->phase  = GEL_IO_ASYNC_READ_OP_PHASE_READ;
	priv->ba     = g_byte_array_new();
	priv->buffer = g_new0(guint8, 32*1024);  // 32kb
	g_input_stream_read_async(priv->stream, priv->buffer, 32*1024,
		G_PRIORITY_DEFAULT, priv->cancellable, _gel_io_async_read_op_read_cb, self);
}

void
_gel_io_async_read_op_read_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) data;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);
	gssize readed;
	GError *err = NULL;

	readed = g_input_stream_read_finish(priv->stream, res, &err);
	if (err != NULL)
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[ERROR], 0, err);
		gel_error("Got error while reading: '%s'", err->message);
		g_error_free(err);
		priv->phase = GEL_IO_ASYNC_READ_OP_PHASE_ERROR;
		return;
	}

	if (readed > 0)
	{
		g_byte_array_append(priv->ba, (const guint8*) priv->buffer, readed);
		g_input_stream_read_async(priv->stream, priv->buffer, 32*1024,
			G_PRIORITY_DEFAULT, priv->cancellable, _gel_io_async_read_op_read_cb, self);
		return;
	}

	priv->phase = GEL_IO_ASYNC_READ_OP_PHASE_CLOSE;
	g_free(priv->buffer);
	g_input_stream_close_async(priv->stream, G_PRIORITY_DEFAULT, priv->cancellable,
		_gel_io_async_read_op_close_cb, self);
}

void
_gel_io_async_read_op_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOAsyncReadOp *self = (GelIOAsyncReadOp *) data;
	GelIOAsyncReadOpPrivate *priv = GET_PRIVATE(self);
	GError *err = NULL;

	if (!g_input_stream_close_finish(priv->stream, res, &err))
	{
		g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[ERROR], 0, err);
		gel_error("Cannot close '%s'", err->message);
		g_error_free(err);
		priv->phase = GEL_IO_ASYNC_READ_OP_PHASE_ERROR;
		return;
	}

	priv->phase = GEL_IO_ASYNC_READ_OP_PHASE_FINISH;
	gel_info("Reached finish state");
	g_file_set_contents("/tmp/lala", (gchar *) priv->ba->data, priv->ba->len, NULL);
	g_object_ref(G_OBJECT(self));
	priv->callback_in_progress = TRUE;
	g_signal_emit(G_OBJECT(self), gel_io_async_read_op_signals[FINISH], 0, priv->ba);
	priv->callback_in_progress = FALSE;
	g_object_unref(G_OBJECT(self));
}

