#ifndef _GEL_BEEF_H
#define _GEL_BEEF_H

#include <gio/gio.h>

typedef struct _GelBeefOp     GelBeefOp;
typedef GNode GelBeefResult;

// Which parameters should be const to avoid user-unref?
typedef void (*GelBeefSuccessFunc) (GelBeefOp *op, const GFile *source, const GelBeefResult *result, gpointer data);
typedef void (*GelBeefErrorFunc)   (GelBeefOp *op, const GFile *source, const GError *error,         gpointer data);

GelBeefOp *
gel_beef_list(GFile *file, const gchar *attributes, gboolean recurse,
  GelBeefSuccessFunc success_cb, GelBeefErrorFunc error_cb, gpointer data);

void
gel_beef_cancel(GelBeefOp *op);

void
gel_beef_free  (GelBeefOp *op);

#endif
