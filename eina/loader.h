#ifndef _EINA_LOADER
#define _EINA_LOADER

#include <glib.h>
#include <eina/base.h>
#include <eina/plugin.h>

#define EINA_LOADER(p)             ((EinaLoader *) p)
#define GEL_HUB_GET_LOADER(hub)    EINA_LOADER(gel_hub_shared_get(hub,"loader"))
#define EINA_BASE_GET_LOADER(base) GEL_HUB_GET_LOADER(EINA_BASE(base)->hub)

G_BEGIN_DECLS

typedef struct _EinaLoader EinaLoader;

/*
 * Loads in a disabled state a plugin from a full path
 */
EinaPlugin*
eina_loader_load_plugin(EinaLoader *self, gchar *pathname, GError **error);

/*
 * Unloads a plugin (only if its in a disabled state)
 */
gboolean
eina_loader_unload_plugin(EinaLoader *self, EinaPlugin *plugin, GError **error);

gboolean
eina_loader_plugin_init(EinaLoader *self, EinaPlugin *plugin, GError **error);

gboolean
eina_loader_plugin_fini(EinaLoader *self, EinaPlugin *plugin, GError **error);

/*
 * Get a plugin via pathname
 */
EinaPlugin*
eina_loader_query_plugin(EinaLoader *self, gchar *pathname);

/*
 * Idem but using name. Dont use libfoo.so schema, use foo instead. Good for you, good
 * for portability
 */
EinaPlugin *
eina_loader_query_plugin_by_name(EinaLoader *self, gchar *name);

/*
 * Checks if plugin is loaded
 */
#define eina_loader_plugin_is_loaded(self,pathname) (eina_loader_query_plugin(self,pathname) != NULL)

/*
 * Returns a list of paths where EinaLoader search for plugins.
 * The returned list must be freed
 */
GList *
eina_loader_query_paths(EinaLoader *self);

/*
 * Returns a list of all available EinaPlugins, whose can be enabled or
 * disabled
 * The returned list must NOT be free'd or altered
 */
GList *
eina_loader_query_plugins(EinaLoader *self);

G_END_DECLS

#endif // _EINA_LOADER
