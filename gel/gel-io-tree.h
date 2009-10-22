#ifndef _GEL_IO_TREE_H
#define _GEL_IO_TREE_H

#include <gio/gio.h>

typedef struct _GelIOTreeOp     GelIOTreeOp;

typedef void (*GelIOTreeSuccessFunc) (GelIOTreeOp *op, const GFile *source, const GNode  *result, gpointer data);
typedef void (*GelIOTreeErrorFunc)   (GelIOTreeOp *op, const GFile *source, const GError *error,  gpointer data);

GelIOTreeOp *
gel_io_tree_walk(GFile *file, const gchar *attributes, gboolean recurse,
  GelIOTreeSuccessFunc success_cb, GelIOTreeErrorFunc error_cb, gpointer data);

void
gel_io_tree_op_close(GelIOTreeOp *op);

GList *
gel_io_tree_result_flatten(const GNode *result);

#endif
