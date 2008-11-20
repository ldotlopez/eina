#ifndef _GEL_IO_SIMPLE
#define _GEL_IO_SIMPLE

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS


/*

typedef void (*GelIOOpSuccessFunc) (GelIOOp *op, gpointer source, GelIOResult *res, gpointer data);
typedef void (*GelIOOpErrorFunc)   (GelIOOp *op, gpointer source, GError *error, gpointer data);


GelIOOp *gel_io_read_file(GFile *file,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);

GelIOOp *gel_io_read_dir (GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);

GelIOOp *gel_io_recurse_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);

GelIOOp *gel_io_recuse_group

void gel_io_op_cancel(GelIOOp *op);

 */

// --
// GelIOSimple unified API
// --
typedef struct _GelIOSimpleOp          GelIOSimpleOp;
typedef struct _GelIOSimpleResult      GelIOSimpleResult;
typedef struct _GelIOSimpleRecurseTree GelIOSimpleRecurseTree;

typedef enum {
	GEL_IO_SIMPLE_RESULT_BYTE_ARRAY,
	GEL_IO_SIMPLE_RESULT_LIST,
	GEL_IO_SIMPLE_RESULT_RECURSE_TREE,
	GEL_IO_SIMPLE_RESULT_GROUP
} GelIOSimpleResultType;

typedef void (*GelIOSimpleOpSuccessFunc)   (GelIOSimpleOp *op, gpointer source, GelIOSimpleResult *res, gpointer data);
typedef void (*GelIOSimpleOpErrorFunc)     (GelIOSimpleOp *op, gpointer source, GError *error, gpointer data);
typedef void (*GelIOSimpleOpCancelledFunc) (GelIOSimpleOp *op, gpointer source, gpointer data);

GelIOSimpleOp *gel_io_simple_read_file(GFile *file, 
	GelIOSimpleOpSuccessFunc   success,
	GelIOSimpleOpErrorFunc     error,
	GelIOSimpleOpCancelledFunc cancelled,
	gpointer data);

GelIOSimpleOp *gel_io_simple_read_dir(GFile *dir, const gchar *attributes,
	GelIOSimpleOpSuccessFunc   success,
	GelIOSimpleOpErrorFunc     error,
	GelIOSimpleOpCancelledFunc cancelled,
	gpointer data);

GelIOSimpleOp *gel_io_simple_recurse_dir(GFile *dir, const gchar *attributes,
	GelIOSimpleOpSuccessFunc   success,
	GelIOSimpleOpErrorFunc     error,
	GelIOSimpleOpCancelledFunc cancelled,
	gpointer data);

gpointer *gel_io_simple_get_source(GelIOSimpleOp *op);
void gel_io_simple_cancel(GelIOSimpleOp *op);

GelIOSimpleResultType  get_io_simple_result_get_type        (GelIOSimpleResult *res);
GByteArray*            gel_io_simple_result_get_byte_array  (GelIOSimpleResult *res);
GList*                 gel_io_simple_result_get_list        (GelIOSimpleResult *res);
GelIOSimpleRecurseTree gel_io_simple_result_get_recurse_tree(GelIOSimpleResult *res);
void                   gel_io_simple_result_free            (GelIOSimpleResult *res);

GelIOSimpleRecurseTree* gel_io_simple_recurse_tree_new(GFile *root);
void                    gel_io_simple_recurse_tree_free(GelIOSimpleRecurseTree *self);

GFile* gel_io_simple_recurse_tree_get_root(GelIOSimpleRecurseTree *self);
GList* gel_io_simple_recurse_tree_get_children(GelIOSimpleRecurseTree *self, GFile *parent);
GList* gel_io_simple_recurse_tree_get_children_as_file(GelIOSimpleRecurseTree *self, GFile *parent);

/* <private> */
void   gel_io_simple_recurse_tree_add_parent(GelIOSimpleRecurseTree *self, GFile *file);
void   gel_io_simple_recurse_tree_add_children(GelIOSimpleRecurseTree *self, GFile *parent, GList *children);
GList* gel_io_simple_recurse_tree_find_in_parents(GelIOSimpleRecurseTree *self, GFile *parent);

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
