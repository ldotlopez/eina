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
