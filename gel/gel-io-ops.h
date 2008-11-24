#ifndef _GEL_IO_SIMPLE
#define _GEL_IO_SIMPLE

#include <glib-object.h>
#include <gio/gio.h>
#include <gel/gel-io-recurse-tree.h>
#include <gel/gel-io-op-result.h>

G_BEGIN_DECLS

typedef struct _GelIOOp          GelIOOp;

typedef void (*GelIOOpSuccessFunc) (GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data);
typedef void (*GelIOOpErrorFunc)   (GelIOOp *op, GFile *source, GError *error, gpointer data);

#define GEL_IO_OP(p)           ((GelIOOp *) p)

GelIOOp *gel_io_read_file(GFile *file,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);

GelIOOp *gel_io_read_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);

GelIOOp *gel_io_recurse_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);

void
gel_io_op_ref(GelIOOp *self);
void
gel_io_op_unref(GelIOOp *self);
void
gel_io_op_destroy(GelIOOp *self);

GFile *
gel_io_op_get_source(GelIOOp *self);
void
gel_io_op_cancel(GelIOOp *self);

#endif
