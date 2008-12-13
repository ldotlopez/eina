#define GEL_DOMAIN "Eina::Loader"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gmodule.h>
#include <gel/gel.h>
#include <eina/loader.h>
#include <eina/player.h>

struct _EinaLoader {
	EinaBase  parent;

	GList *plugins;
	GList *paths;
};

enum {
	PLUGIN_ALREADY_LOADED = 1,
	CANNOT_OPEN_SHARED_OBJECT,
	SYMBOL_NOT_FOUND,
	PLUGIN_NOT_LOADED,
	PLUGIN_IS_ENABLED
};

static GQuark
loader_quark(void);
static GList*
build_paths(void);

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

// Revised
EinaPlugin*
eina_loader_load_plugin(EinaLoader *self, gchar *pathname, GError **error)
{
	EinaPlugin *plugin;

	// Check if it is already loaded
	if ((plugin = eina_loader_query_plugin(self, pathname)))
	{
		g_set_error(error, loader_quark(), PLUGIN_ALREADY_LOADED, N_("Plugin is already loaded"));
		return NULL;
	}

	// Open module
	GModule *module;
	if (!(module = g_module_open(pathname, G_MODULE_BIND_MASK)))
	{
		g_set_error(error, loader_quark(), CANNOT_OPEN_SHARED_OBJECT, N_("Cannot open shared object"));
		return NULL;
	}

	// Create a symbol name for this path
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name    = g_path_get_basename(dirname);
	gchar *symbol  = g_strconcat(name, "_plugin", NULL);
	g_free(dirname);
	g_free(name);

	// Try to retrieve symbol
	if (!(plugin = eina_plugin_new(HUB(self), pathname, symbol)))
	{
		g_set_error(error, loader_quark(), SYMBOL_NOT_FOUND,
			N_("Symbol %s not found in %s"), symbol, pathname);
		g_free(symbol);
		return NULL;
	}

	// Save symbol as loaded and return it
	self->plugins = g_list_append(self->plugins, plugin);
	return plugin;
}

// Revised
gboolean
eina_loader_unload_plugin(EinaLoader *self, EinaPlugin *plugin, GError **error)
{
	// Check if plugin is loaded
	if (!g_list_find(self->plugins, plugin))
	{
		g_set_error(error, loader_quark(), PLUGIN_NOT_LOADED,
			N_("Cannot unload plugin %s: not loaded"), eina_plugin_get_pathname(plugin));
		return FALSE;
	}

	// Check if plugin is not enabled
	if (eina_plugin_is_enabled(plugin))
	{
		g_set_error(error, loader_quark(), PLUGIN_IS_ENABLED,
			N_("Cannot unload plugin %s: is still enabled"), eina_plugin_get_pathname(plugin));
		return FALSE;
	}

	// Delete from ourselves and return
	self->plugins = g_list_remove(self->plugins, plugin);
	eina_plugin_free(plugin);
	return TRUE;
}

EinaPlugin *
eina_loader_query_plugin(EinaLoader *self, gchar *pathname)
{
	GList *iter = self->plugins;

	while (iter)
	{
		if (g_str_equal(
			eina_plugin_get_pathname(EINA_PLUGIN(iter->data)),
			pathname))
		{
			return EINA_PLUGIN(iter->data);
		}

		iter = iter->next;
	}
	return NULL;
}

// Revised
static GQuark
loader_quark(void)
{
	static GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("eina-loader");
	return ret;
}

// Revised
static GList *
build_paths(void)
{
	GList *ret = NULL;

#ifdef PACKAGE_LIB_DIR
	// Add PACKAGE_LIB_DIR
	ret = g_list_prepend(ret, g_strdup(PACKAGE_LIB_DIR));
#endif


#ifdef PACKAGE_NAME
	// Add users dir
	gchar *home = (gchar *) g_getenv ("HOME");
	if (!home)
		home = (gchar *) g_get_home_dir();
	if (home)
		ret = g_list_prepend(ret, g_build_filename(home, "." PACKAGE_NAME, "plugins", NULL));
#endif

#ifdef PACKAGE_NAME
	// Add enviroment variable
	gchar *uc_package_name = g_utf8_strup(PACKAGE_NAME, -1);
	gchar *envvar = g_strdup_printf("%s_PLUGINS_PATH", uc_package_name);
	g_free(uc_package_name);

	const gchar *envval = g_getenv(envvar);
	g_free(envvar);

	if (envval)
	{
		gint i;
		gchar **paths = g_strsplit(envval, ":", 0);
		for (i = 0; paths[i]; i++)
			if (paths[i][0])
				ret = g_list_prepend(ret, paths[i]); // Reuse value
		g_free(paths); // _not_ g_strfreev, we reuse values
	}
#endif

	return ret;
}

// Revised
GList *
eina_loader_query_paths(EinaLoader *self)
{
	return g_list_copy(self->paths);
}

// Revised
GList *
eina_loader_query_plugins(EinaLoader *self)
{
	GList *iter = self->paths;

	// gel_warn("Start query");
	while (iter)
	{
		GList *child, *children;
		child = children = gel_dir_read(iter->data, TRUE, NULL);
		while (child)
		{
			gchar *plugin_name = g_path_get_basename(child->data);
			gchar *module_path = g_module_build_path(child->data, plugin_name);
			g_free(plugin_name);

			// Check if its loaded already and it can be loaded
			if (!eina_loader_plugin_is_loaded(self, module_path))
				if (eina_loader_load_plugin(self, module_path, NULL))
					; // gel_warn("Loaded plugin %s", module_path);

			g_free(module_path);

			child = child->next;
		}
		gel_list_deep_free(children, g_free);

		iter = iter->next;
	}
	/*
	gel_warn("Total available plugins %d", g_list_length(self->plugins));
	iter = self->plugins;
	while (iter)
	{
		gel_warn(" ## [%c] %s",
			eina_plugin_is_enabled(EINA_PLUGIN(iter->data)) ? 'E' : 'D',
			eina_plugin_get_pathname(EINA_PLUGIN(iter->data)));
		iter = iter->next;
	}
	gel_warn("End query");
	*/
	return self->plugins;
}

G_MODULE_EXPORT GelHubSlave loader_connector = {
	"loader",
	&loader_init,
	&loader_exit
};

