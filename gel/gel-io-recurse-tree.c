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
	gel_glist_free((GList *) data, (GFunc) g_object_unref, NULL);
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

	gel_glist_free(self->parents, (GFunc) g_object_unref, NULL);
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

	gchar *uri = g_file_get_uri(p);
	gel_warn("[+] ADD k:%p v:%p (%d:%s)", p, c, g_list_length(c), uri);
	g_free(uri);
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

	GList *c = (GList *) g_hash_table_lookup(self->children, (gpointer) p);

	gchar *uri = g_file_get_uri(p);
	gel_warn("[-] RET k:%p v:%p (%d:%s)", p, c, g_list_length(c), uri);
	g_free(uri);

	return g_list_copy(c);
}

