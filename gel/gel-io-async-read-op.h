#ifndef _GEL_IO_ASYNC_READ_OP
#define _GEL_IO_ASYNC_READ_OP

#include <glib-object.h>

G_BEGIN_DECLS

#define GEL_IO_TYPE_ASYNC_READ_OP gel_io_async_read_op_get_type()

#define GEL_IO_ASYNC_READ_OP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_IO_TYPE_ASYNC_READ_OP, GelIOAsyncReadOp))

#define GEL_IO_ASYNC_READ_OP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GEL_IO_TYPE_ASYNC_READ_OP, GelIOAsyncReadOpClass))

#define GEL_IO_IS_ASYNC_READ_OP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_IO_TYPE_ASYNC_READ_OP))

#define GEL_IO_IS_ASYNC_READ_OP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_IO_TYPE_ASYNC_READ_OP))

#define GEL_IO_ASYNC_READ_OP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_IO_TYPE_ASYNC_READ_OP, GelIOAsyncReadOpClass))

typedef struct {
  GObject parent;
} GelIOAsyncReadOp;

typedef struct {
  GObjectClass parent_class;
  void (*error)  (GelIOAsyncReadOp *self, GError *error);
  void (*finish) (GelIOAsyncReadOp *self, GByteArray *op_data);
} GelIOAsyncReadOpClass;

typedef enum GelIOAsyncReadOpPhase
{
	GEL_IO_ASYNC_READ_OP_PHASE_NONE,
	GEL_IO_ASYNC_READ_OP_PHASE_OPEN,
	GEL_IO_ASYNC_READ_OP_PHASE_READ,
	GEL_IO_ASYNC_READ_OP_PHASE_CLOSE,
	GEL_IO_ASYNC_READ_OP_PHASE_FINISH,
	GEL_IO_ASYNC_READ_OP_PHASE_ERROR
} GelIOAsyncReadOpPhase;

GType gel_io_async_read_op_get_type (void);

GelIOAsyncReadOp*
gel_io_async_read_op_new (gchar *uri);

const gchar *
gel_io_async_read_op_get_buffer(GelIOAsyncReadOp* self);

gboolean
gel_io_async_read_op_cancel(GelIOAsyncReadOp* self);


G_END_DECLS

#endif
