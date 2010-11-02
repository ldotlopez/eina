/*
 * gel/gel-io-recurse-tree.c
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

#define GEL_DOMAIN "Gel::IO::RecurseTree"
#include "gel-io-recurse-tree.h"

struct _GelIORecurseTree {
	guint       refs;
	GList      *parents;
	GHashTable *children;
};

static void
destroy_list_of_file_infos(gpointer data)
{
	gel_list_deep_free((GList *) data, g_object_unref);
}

GelIORecurseTree *
gel_io_recurse_tree_new(void)
{
	GelIORecurseTree *self = g_new0(GelIORecurseTree, 1);

	self->refs     = 1;
	self->parents  = NULL;
	self->children = g_hash_table_new_full(g_direct_hash, NULL,
		g_object_unref, destroy_list_of_file_infos);

	return self;
}

void
gel_io_recurse_tree_ref(GelIORecurseTree *self)
{
	self->refs++;
}

void
gel_io_recurse_tree_unref(GelIORecurseTree *self)
{
	self->refs--;
	if (self->refs > 0)
		return;

	gel_list_deep_free(self->parents, g_object_unref);
	g_hash_table_unref(self->children);

	g_free(self);
}

static GFile*
find_parent(GelIORecurseTree *self, GFile *f)
{
	GList *iter = self->parents;
	while (iter)
	{
		if (g_file_equal(G_FILE(iter->data), f))
			return G_FILE(iter->data);
		iter = iter->next;
	}
	return NULL;
}

void
gel_io_recurse_tree_add_parent(GelIORecurseTree *self, GFile *parent)
{
	g_object_ref(parent);
	self->parents = g_list_prepend(self->parents, parent);
}

void
gel_io_recurse_tree_add_children(GelIORecurseTree *self, GFile *parent, GList *children)
{
	GFile *p = find_parent(self, parent);
	if (p == NULL)
	{
		gel_io_recurse_tree_add_parent(self, parent);
		p = parent;
	}

	GList *c = g_list_copy(children);
	g_list_foreach(c, (GFunc) g_object_ref, NULL);

	g_hash_table_replace(self->children, (gpointer) p, c);
}

GFile*
gel_io_recurse_tree_get_root(GelIORecurseTree *self)
{
	GList *node = g_list_last(self->parents);
	if (node)
		return G_FILE(node->data);
	else
		return NULL;
}

GList*
gel_io_recurse_tree_get_children(GelIORecurseTree *self, GFile *parent)
{
	GFile *p = find_parent(self, parent);
	if (p == NULL)
		return NULL;

	return g_list_copy((GList *) g_hash_table_lookup(self->children, (gpointer) p));
}

