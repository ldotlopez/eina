#ifndef _GRUPPY
#define _GRUPPY

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _GruppyDir GruppyDir;

typedef void (*GruppyDirSuccessFunc)   (GruppyDir *op, GList *results, gpointer data);
typedef void (*GruppyDirErrorFunc)     (GruppyDir *op, GError *error,  gpointer data);
typedef void (*GruppyDirCancelledFunc) (GruppyDir *op, gpointer data);

#define gruppy_dir_read(file,attributes,success,error,cancelled) \
	gruppy_dir_read_full(file,attributes,success,error,cancelled,NULL,NULL)

GruppyDir* gruppy_dir_read_full(GFile *file, const gchar *attributes,
	GruppyDirSuccessFunc   success,
	GruppyDirErrorFunc     error,
	GruppyDirCancelledFunc cancelled,
	gpointer  data,
	GFreeFunc free_func);
void     gruppy_dir_cancel(GruppyDir *gruppy);

void     gruppy_dir_set_data     (GruppyDir *gruppy, gpointer data);
void     gruppy_dir_set_data_full(GruppyDir *gruppy, gpointer data, GFreeFunc free_func);
gpointer gruppy_dir_get_data     (GruppyDir *gruppy);

G_END_DECLS

#endif /* _GRUPPY */
