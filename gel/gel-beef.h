#ifndef _GEL_BEEF_H
#define _GEL_BEEF_H

#include <gio/gio.h>

typedef struct _GelBeefOp     GelBeefOp;

typedef void (*GelBeefSuccessFunc) (GelBeefOp *op, const GFile *source, const GNode  *result, gpointer data);
typedef void (*GelBeefErrorFunc)   (GelBeefOp *op, const GFile *source, const GError *error,  gpointer data);

GelBeefOp *
gel_beef_walk(GFile *file, const gchar *attributes, gboolean recurse,
  GelBeefSuccessFunc success_cb, GelBeefErrorFunc error_cb, gpointer data);

void
gel_beef_close(GelBeefOp *op);

GList *
gel_beef_result_flatten(const GNode *result);

#endif
