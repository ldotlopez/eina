/*
 * gel/gel-io-recurse-tree.h
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

#ifndef _GEL_IO_RECURSE_TREE
#define _GEL_IO_RECURSE_TREE

#include <glib.h>
#include <gio/gio.h>
#include <gel/gel.h>

G_BEGIN_DECLS

typedef struct _GelIORecurseTree GelIORecurseTree;
#define GEL_IO_RECURSE_TREE(p) ((GelIORecurseTree *) p)

GelIORecurseTree*
gel_io_recurse_tree_new(void);
void
gel_io_recurse_tree_ref(GelIORecurseTree *self);
void
gel_io_recurse_tree_unref(GelIORecurseTree *self);

void
gel_io_recurse_tree_add_parent(GelIORecurseTree *self, GFile *parent);
void
gel_io_recurse_tree_add_children(GelIORecurseTree *self, GFile *parent, GList *children);

GFile*
gel_io_recurse_tree_get_root(GelIORecurseTree *self);
GList*
gel_io_recurse_tree_get_children(GelIORecurseTree *self, GFile *file);
GList*
gel_io_recurse_tree_get_children_as_file(GelIORecurseTree *self, GFile *file);


G_END_DECLS
#endif /* _GEL_IO_SIMPLE */
