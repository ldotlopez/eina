#ifndef _GEL_IO_SIMPLE
#define _GEL_IO_SIMPLE

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

// --
// GelIOSimpleFile
// --
typedef struct _GelIOSimpleFile  GelIOSimpleFile;

typedef void (*GelIOSimpleFileSuccessFunc)   (GelIOSimpleFile *self, GByteArray *results, gpointer data);
typedef void (*GelIOSimpleFileErrorFunc)     (GelIOSimpleFile *self, GError *error,  gpointer data);
typedef void (*GelIOSimpleFileCancelledFunc) (GelIOSimpleFile *self, gpointer data);

#define gel_io_simple_file_read(file,success,error,cancelled) \
	gel_io_simple_file_read_full(file,success,error,cancelled,NULL,NULL)
GelIOSimpleFile* gel_io_simple_file_read_full(GFile *file,
	GelIOSimpleFileSuccessFunc   success,
	GelIOSimpleFileErrorFunc     error,
	GelIOSimpleFileCancelledFunc cancelled,
	gpointer  data,
	GFreeFunc free_func);
void     gel_io_simple_file_cancel(GelIOSimpleFile *gruppy);

void     gel_io_simple_file_set_data     (GelIOSimpleFile *gruppy, gpointer data);
void     gel_io_simple_file_set_data_full(GelIOSimpleFile *gruppy, gpointer data, GFreeFunc free_func);
gpointer gel_io_simple_file_get_data     (GelIOSimpleFile *gruppy);

// --
// GelIOSimpleDir
// --
typedef struct _GelIOSimpleDir  GelIOSimpleDir;

typedef void (*GelIOSimpleDirSuccessFunc)   (GelIOSimpleDir *op, GList *results, gpointer data);
typedef void (*GelIOSimpleDirErrorFunc)     (GelIOSimpleDir *op, GError *error,  gpointer data);
typedef void (*GelIOSimpleDirCancelledFunc) (GelIOSimpleDir *op, gpointer data);

#define gel_io_simple_dir_read(file,attributes,success,error,cancelled) \
	gel_io_simple_dir_read_full(file,attributes,success,error,cancelled,NULL,NULL)
GelIOSimpleDir* gel_io_simple_dir_read_full(GFile *file, const gchar *attributes,
	GelIOSimpleDirSuccessFunc   success,
	GelIOSimpleDirErrorFunc     error,
	GelIOSimpleDirCancelledFunc cancelled,
	gpointer  data,
	GFreeFunc free_func);
void     gel_io_simple_dir_cancel(GelIOSimpleDir *self);

void     gel_io_simple_dir_set_data     (GelIOSimpleDir *self, gpointer data);
void     gel_io_simple_dir_set_data_full(GelIOSimpleDir *self, gpointer data, GFreeFunc free_func);
gpointer gel_io_simple_dir_get_data     (GelIOSimpleDir *self);

G_END_DECLS

#endif /* _GEL_IO_SIMPLE */
