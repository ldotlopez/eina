#ifndef _GEL_IO_OP_RESULT
#define _GEL_IO_OP_RESULT

#include <glib.h>
#include <gio/gio.h>
#include <gel/gel-io-recurse-tree.h>

G_BEGIN_DECLS

typedef struct _GelIOOpResult    GelIOOpResult;
typedef enum {
	GEL_IO_OP_RESULT_BYTE_ARRAY,
	GEL_IO_OP_RESULT_OBJECT_LIST,
	GEL_IO_OP_RESULT_RECURSE_TREE
} GelIOOpResultType;

#define GEL_IO_OP(p)           ((GelIOOp *) p)
#define GEL_IO_OP_RESULT(p)    ((GelIOOpResult *) p)

GelIOOpResult*
gel_io_op_result_new(GelIOOpResultType type, gpointer result);
void
gel_io_op_result_destroy(GelIOOpResult *self);

void
gel_io_op_result_ref(GelIOOpResult *self);
void
gel_io_op_result_unref(GelIOOpResult *self);

GelIOOpResultType
gel_io_op_result_get_type(GelIOOpResult *self);

GByteArray *
gel_io_op_result_get_byte_array(GelIOOpResult *self);
GList*
gel_io_op_result_get_object_list(GelIOOpResult *self);
GelIORecurseTree*
gel_io_op_result_get_recurse_tree(GelIOOpResult *self);

#endif // _GEL_IO_OP_RESULT

