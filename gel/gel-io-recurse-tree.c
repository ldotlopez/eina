#define GEL_DOMAIN "Gel::IO::RecurseTree"
#include <gel/gel-io-recurse-tree.h>

struct _GelIORecurseTree {
	guint       refs;
	GList      *parents;
	GHashTable *children;
};

static void
destroy_list_of_file_infos(gpointer data)
{
	gel_glist_free((GList *) data, (GFunc) g_free, NULL);
}

GelIORecurseTree *
gel_io_recurse_tree_new(void)
{
	GelIORecurseTree *self = g_new0(GelIORecurseTree, 1);

	self->refs     = 1;
	self->parents  = NULL;
	self->children = g_hash_table_new_full(g_direct_hash, (GEqualFunc) g_file_equal,
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

	gel_glist_free(self->parents, (GFunc) g_object_unref, NULL);
	g_hash_table_unref(self->children);

	g_free(self);
}

void
gel_io_recurse_tree_add_parent(GelIORecurseTree *self, GFile *parent)
{
	self->parents = g_list_prepend(self->parents, parent);
	g_hash_table_insert(self->children, (gpointer) parent, NULL);
}

void
gel_io_recurse_tree_add_children(GelIORecurseTree *self, GFile *parent, GList *children)
{
	if (!g_list_find(self->parents, parent))
		gel_io_recurse_tree_add_parent(self, parent);
	g_hash_table_replace(self->children, (gpointer) parent, children);
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
	return (GList *) g_hash_table_lookup(self->children, (gpointer) parent);
}

