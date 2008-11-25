#define GEL_DOMAIN "Gel::IO::OpResult"
#include <gel/gel-io-op-result.h>

struct _GelIOOpResult {
	// guint             refs;
	GelIOOpResultType type;
	gpointer          result;
};

GelIOOpResult*
gel_io_op_result_new(GelIOOpResultType type, gpointer result)
{
	GelIOOpResult *self = g_new0(GelIOOpResult, 1);
	// self->refs   = 1;
	switch (self->type)
	{
	case GEL_IO_OP_RESULT_BYTE_ARRAY:
	case GEL_IO_OP_RESULT_OBJECT_LIST:
	case GEL_IO_OP_RESULT_RECURSE_TREE:
		break;

	default:
		g_free(self);
		gel_error("Unknow type %d for GelIOOpResult", type);
		return NULL;
	}

	self->type   = type;
	self->result = (gpointer) result;
	return self;
}

#if 0
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
		break;
	case GEL_IO_OP_RESULT_OBJECT_LIST:
		break;
	case GEL_IO_OP_RESULT_RECURSE_TREE:
		break;
	default:
		gel_error("Unknow type %d for GelIOOpResult", self->type);
		break;
	}
	g_free(self);
}
#endif

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

