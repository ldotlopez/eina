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

typedef void (*GelIOSimpleDirSuccessFunc)   (GelIOSimpleDir *op, GFile *parent, GList *results, gpointer data);
typedef void (*GelIOSimpleDirErrorFunc)     (GelIOSimpleDir *op, GFile *parent, GError *error,  gpointer data);
typedef void (*GelIOSimpleDirCancelledFunc) (GelIOSimpleDir *op, GFile *parent, gpointer data);

GelIOSimpleDir* gel_io_simple_dir_read(GFile *file, const gchar *attributes,
	GelIOSimpleDirSuccessFunc   success,
	GelIOSimpleDirErrorFunc     error,
	GelIOSimpleDirCancelledFunc cancelled,
	gpointer  data);
void     gel_io_simple_dir_close(GelIOSimpleDir *self);
void     gel_io_simple_dir_cancel(GelIOSimpleDir *self);

// --
// Recusirve operation
// --
typedef struct _GelIOSimpleDirRecurse GelIOSimpleDirRecurse;
typedef struct _GelIOSimpleDirRecurseResult GelIOSimpleDirRecurseResult;

typedef void (*GelIOSimpleDirRecurseSuccessFunc)   (GelIOSimpleDirRecurse *op, GFile *parent, GelIOSimpleDirRecurseResult *res, gpointer data);
typedef void (*GelIOSimpleDirRecurseErrorFunc)     (GelIOSimpleDirRecurse *op, GFile *parent, GError *error,  gpointer data);
typedef void (*GelIOSimpleDirRecurseCancelledFunc) (GelIOSimpleDirRecurse *op, GFile *parent, gpointer data);

GelIOSimpleDirRecurse* gel_io_simple_dir_recurse_read(GFile *file, const gchar *attributes,
	GelIOSimpleDirRecurseSuccessFunc   success,
	GelIOSimpleDirRecurseErrorFunc     error,
	GelIOSimpleDirRecurseCancelledFunc cancelled,
	gpointer  data);
void  gel_io_simple_dir_recurse_close(GelIOSimpleDirRecurse *op);

void gel_io_simple_dir_recurse_cancel(GelIOSimpleDirRecurse *op);

// --
// Recursive result
// --
GFile *gel_io_simple_dir_recurse_result_get_root            (GelIOSimpleDirRecurseResult *res);
GList *gel_io_simple_dir_recurse_result_get_children        (GelIOSimpleDirRecurseResult *res, GFile *node);
GList *gel_io_simple_dir_recurse_result_get_children_as_file(GelIOSimpleDirRecurseResult *res, GFile *node);
void   gel_io_simple_dir_recurse_result_free                (GelIOSimpleDirRecurseResult *res);

// --
// Group
// --
typedef struct _GelIOSimpleGroup GelIOSimpleGroup;
typedef void (*GelIOSimpleGroupSuccessFunc)   (GelIOSimpleGroup *op, GList *group, GList *results, gpointer data);
typedef void (*GelIOSimpleGroupErrorFunc)     (GelIOSimpleGroup *op, GList *group, GFile *source, gpointer data);
typedef void (*GelIOSimpleGroupCancelledFunc) (GelIOSimpleGroup *op, GList *group, gpointer data);

GelIOSimpleGroup *gel_io_simple_group_read(GList *group, const gchar *attributes, guint flags,
	GelIOSimpleGroupSuccessFunc   success,
	GelIOSimpleGroupErrorFunc     error,
	GelIOSimpleGroupCancelledFunc cancelled,
	gpointer data);
void gel_io_simple_group_free  (GelIOSimpleGroup *self);
void gel_io_simple_group_cancel(GelIOSimpleGroup *self);

G_END_DECLS

#endif /* _GEL_IO_SIMPLE */
