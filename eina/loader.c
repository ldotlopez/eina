#define GEL_DOMAIN "Eina::Loader"

#include "base.h"
#include "loader.h"
#include <gmodule.h>

struct _EinaLoader {
	EinaBase  parent;
	GList    *plugins;
	GList    *paths;
};

static GList*
build_paths(void);

G_MODULE_EXPORT gboolean loader_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaLoader *self;

	// Create mem in hub (if needed)
	self = g_new0(EinaLoader, 1);
	if (!eina_base_init((EinaBase *) self, hub, "loader", EINA_BASE_NONE))
	{
		gel_error("Cannot create component");
		return FALSE;
	}

	self->paths = build_paths();

	return TRUE;
}

G_MODULE_EXPORT gboolean loader_exit
(gpointer data)
{
	EinaLoader *self = EINA_LOADER(data);
	GList *iter;

	// Unload all plugins. All plugins must be disabled while unloading
	iter = self->plugins;
	while (iter)
	{
		if (eina_plugin_is_enabled(EINA_PLUGIN(iter->data)))
			break;
		if (!eina_plugin_free(EINA_PLUGIN(iter->data)))
			break;
	}
	if (iter != NULL)
	{
		gel_error("Cannot exit: not all plugins are disabled (%s)", eina_plugin_get_pathname(EINA_PLUGIN(iter->data)));
		return FALSE;
	}
	g_list_free(self->plugins);

	// Free paths
	g_list_foreach(self->paths, (GFunc) g_free, NULL);
	g_list_free(self->paths);

	eina_base_fini(EINA_BASE(self));

	return TRUE;
}

static GList *
build_paths(void)
{
	GList *ret = NULL;
	gchar *tmp;
	gchar **tmp2;
	gint i;

	// Add system dir
	ret = g_list_prepend(ret, g_strdup(PACKAGE_LIB_DIR));

	// Add user dir
	tmp = (gchar *) g_getenv ("HOME");
	if (!tmp)
		tmp = (gchar *) g_get_home_dir();
	if (tmp)
		ret = g_list_prepend(ret, g_build_filename(tmp, ".eina", "plugins", NULL));

	// Add from $EINA_PLUGINS_PATH
	tmp = (gchar *) g_getenv("EINA_PLUGINS_PATH");
	if (tmp != NULL)
	{
		tmp2 = g_strsplit(tmp, ":", 0);
		for (i = 0; tmp2[i] != NULL; i++) {
			if (tmp2[i][0] == '\0')
				continue;
			ret = g_list_prepend(ret, g_strdup(tmp2[i]));
		}
		g_strfreev(tmp2);
	}

	return ret;
}

GList *
eina_loader_query_plugins(EinaLoader *self)
{
	return NULL;
}

EinaPlugin*
eina_loader_load_plugin(EinaLoader *self, gchar *pathname, GError **error)
{
	return NULL;
}

gboolean
eina_loader_unload_plugin(EinaLoader *self, EinaPlugin *plugin, GError **error)
{
	return FALSE;
}

G_MODULE_EXPORT GelHubSlave loader_connector = {
	"loader",
	&loader_init,
	&loader_exit
};

