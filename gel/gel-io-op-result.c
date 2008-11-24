#define GEL_DOMAIN "Gel::IO::OpResult"
#include <gel/gel-io-op-result.h>

struct _GelIOOpResult {
	guint             refs;
	GelIOOpResultType type;
	gpointer          result;
};

GelIOOpResult*
gel_io_op_result_new(GelIOOpResultType type, gpointer result)
{
	GelIOOpResult *self = g_new0(GelIOOpResult, 1);
	self->refs   = 1;
	self->type   = type;
	self->result = result;
	return self;
}

void
gel_io_op_result_ref(GelIOOpResult *self)
{
	self->refs++;
}

void
gel_io_op_result_unref(GelIOOpResult *self)
{
	self->refs--;
	if (self->refs > 0)
		return;
	gel_io_op_result_destroy(self);
}

void
gel_io_op_result_destroy(GelIOOpResult *self)
{
	switch (self->type)
	{
	case GEL_IO_OP_RESULT_BYTE_ARRAY:
		g_byte_array_free((GByteArray*) self->result, TRUE);
		break;
	case GEL_IO_OP_RESULT_OBJECT_LIST:
		gel_glist_free((GList *) self->result, (GFunc) g_object_unref, NULL);
		break;
	case GEL_IO_OP_RESULT_RECURSE_TREE:
		gel_io_recurse_tree_unref(GEL_IO_RECURSE_TREE(self->result));
		break;
	default:
		gel_error("Unknow type %d for GelIOOpResult", self->type);
		break;
	}
	g_free(self);
}

GelIOOpResultType
gel_io_op_result_get_type(GelIOOpResult *self)
{
	return self->type;
}

GByteArray *
gel_io_op_result_get_byte_array(GelIOOpResult *self)
{
	if (!self || (self->type != GEL_IO_OP_RESULT_BYTE_ARRAY))
		return NULL;
	return (GByteArray*) self->result;
}

GList*
gel_io_op_result_get_object_list(GelIOOpResult *self)
{
	if (!self || (self->type != GEL_IO_OP_RESULT_OBJECT_LIST))
		return NULL;
	return (GList *) self->result;
}

GelIORecurseTree*
gel_io_op_result_get_recurse_tree(GelIOOpResult *self)
{
	if (!self || (self->type != GEL_IO_OP_RESULT_RECURSE_TREE))
		return NULL;
	return (GelIORecurseTree*) self->result;
}

