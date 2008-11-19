#ifndef _EINA_LOADER
#define _EINA_LOADER

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <eina/plugin.h>

#define GEL_HUB_GET_LOADER(hub)    gel_hub_shared_get(hub,"loader")
#define EINA_BASE_GET_LOADER(base) GEL_HUB_GET_LOADER(((EinaBase *)base)->hub)
#define EINA_LOADER(p)             ((EinaLoader *) p)

G_BEGIN_DECLS

typedef struct _EinaLoader EinaLoader;

/*
 * Returns a list of all available EinaPlugins, whose can be enabled or
 * disabled
 */
GList *
eina_loader_query_plugins(EinaLoader *self);

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

G_END_DECLS

#endif // _EINA_LOADER
