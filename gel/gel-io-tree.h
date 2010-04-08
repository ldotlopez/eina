/*
 * gel/gel-io-tree.h
 *
 * Copyright (C) 2004-2010 Eina
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


#ifndef _GEL_IO_TREE_H
#define _GEL_IO_TREE_H

#include <gio/gio.h>

typedef struct _GelIOTreeOp     GelIOTreeOp;

typedef void (*GelIOTreeSuccessFunc) (GelIOTreeOp *op, const GFile *source, const GNode  *result, gpointer data);
typedef void (*GelIOTreeErrorFunc)   (GelIOTreeOp *op, const GFile *source, const GError *error,  gpointer data);

#define gel_file_get_file_info(file) G_FILE_INFO(g_object_get_data((GObject *) file, "gfileinfo"))

GelIOTreeOp *
gel_io_tree_walk(GFile *file, const gchar *attributes, gboolean recurse,
  GelIOTreeSuccessFunc success_cb, GelIOTreeErrorFunc error_cb, gpointer data);

GCancellable*
gel_io_tree_op_get_cancellable(GelIOTreeOp *op);

void
gel_io_tree_op_close(GelIOTreeOp *op);

GList *
gel_io_tree_result_flatten(const GNode *result);

#endif
