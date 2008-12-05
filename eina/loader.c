#define GEL_DOMAIN "Eina::Loader"

#include <gmodule.h>
#include <gel/gel.h>
#include <eina/loader.h>
#include <eina/player.h>

struct _EinaLoader {
	EinaBase  parent;

	GList *plugins;
	GList *paths;

// 	EinaPlayer     *player;
};

static GList*
build_paths(void);
static EinaPlugin *
find_plugin(EinaLoader *self, const gchar *pathname);

G_MODULE_EXPORT gboolean loader_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaLoader *self;

	self = g_new0(EinaLoader, 1);
	if (!eina_base_init(EINA_BASE(self), hub, "loader", EINA_BASE_NONE))
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
		if (eina_plugin_is_enabled(EINA_PLUGIN(iter->data)) && !eina_plugin_fini(EINA_PLUGIN(iter->data)))
			break;
		if (!eina_plugin_free(EINA_PLUGIN(iter->data)))
			break;

		iter = iter->next;
	}

	if (iter != NULL)
	{
		gel_error("Cannot exit: not all plugins are disabled (%s)", eina_plugin_get_pathname(EINA_PLUGIN(iter->data)));
		return FALSE;
	}
	g_list_free(self->plugins);

	// Free paths
	gel_list_deep_free(self->paths, g_free);

	eina_base_fini(EINA_BASE(self));

	return TRUE;
}

EinaPlugin*
eina_loader_load_plugin(EinaLoader *self, gchar *pathname, GError **error)
{
	EinaPlugin *plugin;

	// Already loaded
	if ((plugin = find_plugin(self, pathname)) != NULL)
		return plugin;
	
	// Create a symbol name for this path
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name = g_path_get_basename(dirname);
	gchar *symbol = g_strconcat(name, "_plugin", NULL);
	g_free(dirname);
	g_free(name);

	// Load plugin
	if ((plugin = eina_plugin_new(HUB(EINA_BASE(self)), pathname, symbol)) == NULL)
	{
		g_free(symbol);
		return NULL;
	}

	// Add to our list
	self->plugins = g_list_prepend(self->plugins, plugin);

	return plugin;
}

gboolean
eina_loader_unload_plugin(EinaLoader *self, EinaPlugin *plugin, GError **error)
{
	// Check for plugin
	if (g_list_find(self->plugins, plugin) == NULL)
		return TRUE;

	// Check if its enabled
	if (eina_plugin_is_enabled(plugin))
		return FALSE;
	
	// Remove plugin
	self->plugins = g_list_remove(self->plugins, plugin);
	eina_plugin_free(plugin);

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

static EinaPlugin *
find_plugin(EinaLoader *self, const gchar *pathname)
{
	GList *iter = self->plugins;
	while (iter)
	{
		if (g_str_equal(
			eina_plugin_get_pathname(EINA_PLUGIN(iter->data)),
			pathname))
			return EINA_PLUGIN(iter->data);
		iter = iter->next;
	}
	return NULL;
}

GList *
eina_loader_query_paths(EinaLoader *self)
{
	return self->paths;
}

GList *
eina_loader_query_plugins(EinaLoader *self)
{
	GList *iter;
	GList *ret = NULL;

	iter = eina_loader_query_paths(self);
	while (iter)
	{
		GList *e, *entries;
		e = entries = gel_dir_read(iter->data, TRUE, NULL);
		while (e)
		{	
			EinaPlugin *plugin;
			gchar *name = g_path_get_basename(e->data);
			gchar *mod_name = g_module_build_path(e->data, name);

			if ((plugin = eina_loader_load_plugin(self, mod_name, NULL)) != NULL)
			{
				gel_info("Loaded '%s'", mod_name);
				ret = g_list_prepend(ret, plugin);
			}
			else
				gel_info("Cannot load '%s'", mod_name);

			g_free(mod_name);
			g_free(name);
			e = e->next;
		}
		gel_list_deep_free(entries, g_free);

		iter = iter->next;
	}

	// Replace current plugin list
	g_list_free(self->plugins);
	self->plugins = g_list_reverse(ret);

	return self->plugins;
}

G_MODULE_EXPORT GelHubSlave loader_connector = {
	"loader",
	&loader_init,
	&loader_exit
};

