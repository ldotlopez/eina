#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <gel/gel-misc.h>
#include <gel/gel-app.h>
#include <gel/gel-app-marshal.h>

G_DEFINE_TYPE (GelApp, gel_app, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_TYPE_APP, GelAppPrivate))

static GQuark
gel_app_quark(void);
static GList*
build_paths(void);

#define is_owned(self,plugin) \
	((gel_plugin_get_app(plugin) == self) && (g_list_find(self->priv->plugins, plugin) != NULL))

struct _GelAppPrivate {
	GelAppDisposeFunc dispose_func;
	gpointer  dispose_data;

	GList      *plugins; // Loaded plugins
	GHashTable *shared;  // Shared memory
	GList      *paths;   // Paths to search plugins
};

enum {
	PLUGIN_INIT,
	PLUGIN_FINI,
	PLUGIN_LOAD,
	PLUGIN_UNLOAD,
	// MODULE_REF,
	// MODULE_UNREF,

	LAST_SIGNAL
};
static guint gel_app_signals[LAST_SIGNAL] = { 0 };

static gchar*
print_plugin(const gpointer p)
{
	return g_strdup(gel_plugin_stringify(GEL_PLUGIN(p)));
}

static void
gel_app_dispose (GObject *object)
{
	GelApp *self = GEL_APP(object);

	if (self->priv != NULL)
	{
		// Call dispose before destroy us
		if (self->priv->dispose_func)
		{	
			self->priv->dispose_func(self, self->priv->dispose_data);
			self->priv->dispose_func = NULL;
		}

		// Unload all plugins in reverse order
		g_printf("Deleting plugins\n");
		if (self->priv->plugins)
		{
			GList *iter = self->priv->plugins = g_list_reverse(self->priv->plugins);
			gel_list_printf(self->priv->plugins, "%s\n", print_plugin);
			GError *error = NULL;

			while (iter)
			{
				GelPlugin *plugin = GEL_PLUGIN(iter->data);
				g_printf("Deleting plugin: %s\n", plugin->name);
				if (gel_plugin_is_enabled(plugin) && !gel_app_fini_plugin(self, plugin, &error))
				{
					g_warning("Cannot fini plugin '%s': %s\n", gel_plugin_stringify(plugin), error->message);
					g_error_free(error);
					iter = iter->next; continue;
				}
				if (!gel_app_unload_plugin(self, plugin, &error))
				{
					g_warning("Cannot fini plugin '%s': %s\n", gel_plugin_stringify(plugin), error->message);
					g_error_free(error);
					iter = iter->next; continue;
				}

				iter = iter->next;
			}
			self->priv->plugins = NULL;
		}

		// Clear paths
		if (self->priv->paths)
		{
			gel_list_deep_free(self->priv->paths, g_free);
			self->priv->paths = NULL;
		}

		gel_free_and_invalidate(self->priv->shared, NULL, g_hash_table_destroy);
		gel_free_and_invalidate(self->priv, NULL, g_free);
	}

	G_OBJECT_CLASS (gel_app_parent_class)->dispose (object);
}

static void
gel_app_finalize (GObject *object)
{
	G_OBJECT_CLASS (gel_app_parent_class)->finalize (object);
}

static void
gel_app_class_init (GelAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	// g_type_class_add_private (klass, sizeof (GelAppPrivate));

	object_class->dispose = gel_app_dispose;
	object_class->finalize = gel_app_finalize;

    gel_app_signals[PLUGIN_INIT] = g_signal_new ("plugin-init",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelAppClass, plugin_init),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    gel_app_signals[PLUGIN_FINI] = g_signal_new ("plugin-fini",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelAppClass, plugin_fini),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
	/* Load / unload */
    gel_app_signals[PLUGIN_LOAD] = g_signal_new ("plugin-load",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelAppClass, plugin_load),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    gel_app_signals[PLUGIN_UNLOAD] = g_signal_new ("plugin-unload",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelAppClass, plugin_unload),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
	/* ref / unref */
	/*
    gel_app_signals[MODULE_REF] = g_signal_new ("plugin-ref",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelAppClass, plugin_ref),
        NULL, NULL,
        gel_app_marshal_VOID__POINTER_UINT,
        G_TYPE_NONE,
        2,
		G_TYPE_POINTER,
		G_TYPE_UINT);
    gel_app_signals[MODULE_UNREF] = g_signal_new ("plugin-unref",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelAppClass, plugin_unref),
        NULL, NULL,
        gel_app_marshal_VOID__POINTER_UINT,
        G_TYPE_NONE,
        2,
		G_TYPE_POINTER,
		G_TYPE_UINT);
	*/
}

static void
gel_app_init (GelApp *self)
{
	GelAppPriv *priv = self->priv = g_new0(GelAppPriv, 1);

	priv->dispose_func = priv->dispose_data = NULL;
	priv->plugins = priv->paths = NULL;
	priv->shared = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

GelApp*
gel_app_new (void)
{
	return g_object_new (GEL_TYPE_APP, NULL);
}

void
gel_app_set_dispose_callback(GelApp *self, GelAppDisposeFunc callback, gpointer user_data)
{
	g_warn_if_fail(self->priv->dispose_func == NULL);
	g_warn_if_fail(callback != NULL);

	self->priv->dispose_func = callback;
	self->priv->dispose_data = user_data;
}

GelPlugin*
gel_app_load_plugin(GelApp *self, gchar *pathname, GError **error)
{
	GelPlugin *plugin;

	// Create a symbol name for this path
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name    = g_path_get_basename(dirname);
	gchar *symbol  = g_strconcat(name, "_plugin", NULL);
	g_free(dirname);
	g_free(name);

	plugin = gel_app_load_plugin_full(self, pathname, symbol, error);

	g_free(symbol);
	return plugin;
}

GelPlugin *
gel_app_load_buildin(GelApp *self, gchar *symbol, GError **error)
{
	return gel_app_load_plugin_full(self, NULL, symbol, error);
}

GelPlugin *
gel_app_load_plugin_full(GelApp *self, gchar *pathname, gchar *symbol, GError **error)
{
	GelPlugin *plugin;

	if ((plugin =  gel_app_query_plugin(self, pathname, symbol)) != NULL)
	{
		gel_plugin_ref(plugin);
	}
	else
	{
		// Create the plugin
		if ((plugin = gel_plugin_new(self, pathname, symbol, error)) == NULL)
			return NULL;

		g_signal_emit(self, gel_app_signals[PLUGIN_LOAD], 0, plugin);

		// Save symbol as loaded and return it
		self->priv->plugins = g_list_append(self->priv->plugins, plugin);
	}
	return plugin;
}

gboolean
gel_app_unload_plugin(GelApp *self, GelPlugin *plugin, GError **error)
{
	// Plugin is owned by us?
	if (!is_owned(self, plugin))
	{
		g_set_error(error, gel_app_quark(), GEL_APP_NO_OWNED_PLUGIN,
			N_("Plugin '%s' is not owned by app %p"), gel_plugin_stringify(plugin), self);
		return FALSE;
	}

	// Dont fini it explictly
	if (gel_plugin_is_enabled(plugin))
	{
		gel_plugin_unref(plugin);
		/*
		g_set_error(error, gel_app_quark(), GEL_APP_PLUGIN_STILL_ENABLED,
			N_("Plugin %s is still enabled, cannot unload it"), gel_plugin_stringify(plugin));
		*/
		return TRUE;
	}

	// unref it, if there are no references around free it
	gel_plugin_unref(plugin);
	if (gel_plugin_get_loads(plugin) >= 0)
		return TRUE;

	// Delete from internal list, if there is (should not append) an error
	// re-add it
	g_signal_emit(self, gel_app_signals[PLUGIN_UNLOAD], 0, plugin);
	self->priv->plugins = g_list_remove(self->priv->plugins, plugin);
	if (!gel_plugin_free(plugin, error))
	{
		g_signal_emit(self, gel_app_signals[PLUGIN_LOAD], 0, plugin);
		self->priv->plugins = g_list_append(self->priv->plugins, plugin);
		return FALSE;
	}

	return TRUE;
}

GelPlugin *
gel_app_load_plugin_by_name(GelApp *self, gchar *plugin_name)
{
	GelPlugin *ret = NULL;
	GList *iter = self->priv->paths;
	while (iter)
	{
		gchar *path = (gchar*) iter->data;
		gchar *plugin_dirname = g_build_filename(path, plugin_name, NULL);
		gchar *plugin_pathname = g_module_build_path(plugin_dirname, plugin_name);
		g_free(plugin_dirname);

		GelPlugin *plugin = gel_app_load_plugin(self, plugin_pathname, NULL);
		g_free(plugin_pathname);

		if (plugin != NULL)
			break;

		iter = iter->next;
	}

	// Try build-in feature if not found
	if (iter == NULL)
	{
		gchar *symbol = g_strconcat(plugin_name, "_plugin", NULL);
		ret = gel_app_load_buildin(self, symbol, NULL);
		g_free(symbol);
	}

	return ret;
}

gboolean
gel_app_init_plugin(GelApp *self, GelPlugin *plugin, GError **error)
{
	if (!is_owned(self, plugin))
	{
		g_set_error(error, gel_app_quark(), GEL_APP_NO_OWNED_PLUGIN,
			N_("Plugin '%s' is not owned by app %p"), gel_plugin_stringify(plugin), self);
		return FALSE;
	}

	// If this init was the first, emit signal
	gboolean initialized = gel_plugin_init(plugin, error);
	if (initialized && (gel_plugin_get_inits(plugin) == 1))
	{
		g_signal_emit(self, gel_app_signals[PLUGIN_INIT], 0, plugin);
	}

	return initialized;
}

gboolean
gel_app_fini_plugin(GelApp *self, GelPlugin *plugin, GError **error)
{
	if (!is_owned(self, plugin))
	{
		g_set_error(error, gel_app_quark(), GEL_APP_NO_OWNED_PLUGIN,
			N_("Plugin '%s' is not owned by app %p"), gel_plugin_stringify(plugin), self);
		return FALSE;
	}

	// If this fini was the last, emit signal
	gboolean finalized = gel_plugin_fini(plugin, error);
	if (finalized && (gel_plugin_get_inits(plugin) == 0))
	{
		g_signal_emit(self, gel_app_signals[PLUGIN_FINI], 0, plugin);
	}

	return finalized;
}

GList *
gel_app_query_plugins(GelApp *self)
{
    GList *paths = gel_app_query_paths(self);
	GList *iter  = paths;

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
			/*
			if (!gel_app_query_plugin_by_pathname(self, module_path))
				if (gel_app_load_plugin(self, module_path, NULL))
					; // gel_warn("Loaded plugin %s", module_path);
			*/
			gel_app_load_plugin(self, module_path, NULL);
			g_free(module_path);

			child = child->next;
		}
		gel_list_deep_free(children, g_free);

		iter = iter->next;
	}
	g_list_free(paths);

    return g_list_copy(self->priv->plugins);
}

GList *
gel_app_query_paths(GelApp *self)
{
	gel_list_deep_free(self->priv->paths, g_free);
	self->priv->paths = build_paths();
	return g_list_copy(self->priv->paths);
}

GelPlugin*
gel_app_query_plugin(GelApp *self, gchar *pathname, gchar *symbol)
{
	GList *iter = self->priv->plugins;
	while (iter)
	{
		if (gel_plugin_matches(GEL_PLUGIN(iter->data), pathname, symbol))
			return  GEL_PLUGIN(iter->data);
		iter = iter->next;
	}
	return NULL;
}

GelPlugin *
gel_app_query_plugin_by_pathname(GelApp *self, gchar *pathname)
{
	// Build symbolname
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *plugin_name = g_path_get_basename(pathname);
	g_free(dirname);
	gchar *symbol_name = g_strconcat(plugin_name, "_plugin", NULL);
	g_free(plugin_name);

	GelPlugin *ret = gel_app_query_plugin(self, pathname, symbol_name);
	g_free(symbol_name);

	return ret;
}

GelPlugin *
gel_app_query_plugin_by_name(GelApp *self, gchar *name)
{
	GList *iter = self->priv->plugins;
	while (iter)
	{
		if (g_str_equal(GEL_PLUGIN(iter->data)->name, name))
			return GEL_PLUGIN(iter->data);
		iter = iter->next;
	}
	return NULL;
}

gboolean gel_app_shared_register(GelApp *self, gchar *name, gsize size)
{
	if (gel_app_shared_get(self, name) != NULL)
		return FALSE;
	return gel_app_shared_set(self, name, g_malloc0(size));
}

gboolean
gel_app_shared_unregister(GelApp *self, gchar *name)
{
	if (gel_app_shared_get(self, name) == NULL)
		return TRUE;

	return g_hash_table_remove(self->priv->shared, name);
}

gboolean gel_app_shared_set
(GelApp *self, gchar *name, gpointer data)
{
	if (gel_app_shared_get(self, name) != NULL)
		return FALSE;
	g_hash_table_insert(self->priv->shared, g_strdup(name), data);

	return TRUE;
}

gpointer gel_app_shared_get
(GelApp *self, gchar *name)
{
	return g_hash_table_lookup(self->priv->shared, name);
	/*
	g_printf("Request for %s: %p\n", name, ret);
	if (ret == NULL)
	{
		g_printf("Key not found, currently:\n");
		GList *keys = g_hash_table_get_keys(self->priv->shared);
		GList *iter = keys;
		while (iter)
		{
			g_printf("+ %s\n", (gchar *) iter->data);
			iter = iter->next;
		}
		g_list_free(keys);
	}

	return ret; */
}

// --
// Statics
// --
static GQuark 
gel_app_quark(void)
{
	static GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("gel-app");
	return ret;
}


static GList*
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

