/*
 * gel/gel-io-op-result.h
 *
 * Copyright (C) 2004-2009 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

/*
This is just a proxy class, just a set of methods to access
to type and data of a result packed togheter.
Dont free them, dont free object, just access them
*/

#ifdef GEL_COMPILATION // Use only inside libgel
GelIOOpResult*
gel_io_op_result_new(GelIOOpResultType type, gpointer result);
#endif

// Unimplemented methods
#if 0
void
gel_io_op_result_destroy(GelIOOpResult *self);

void
gel_io_op_result_ref(GelIOOpResult *self);
void
gel_io_op_result_unref(GelIOOpResult *self);
#endif

GelIOOpResultType
gel_io_op_result_get_type(GelIOOpResult *self);

GByteArray *
gel_io_op_result_get_byte_array(GelIOOpResult *self);
GList*
gel_io_op_result_get_object_list(GelIOOpResult *self);
GelIORecurseTree*
gel_io_op_result_get_recurse_tree(GelIOOpResult *self);

#endif // _GEL_IO_OP_RESULT

