/*
 * gel/gel-app.c
 *
 * Copyright (C) 2004-2009 Eina
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

#define GEL_DOMAIN "Eina::Plugins"
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
gel_app_quark(void)
{
	static GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("gel-app");
	return ret;
}

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
			// GError *error = NULL;

			while (iter)
			{
			/*
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
			*/
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
	priv->plugins = NULL;
	priv->paths  = NULL;
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

static gchar *
symbol_from_pathname(gchar *pathname)
{
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *plugin_name = g_path_get_basename(dirname);
	g_free(dirname);
	gchar *symbol_name = g_strconcat(plugin_name, "_plugin", NULL);
	g_free(plugin_name);
	return symbol_name;
}

GList *
gel_app_query_paths(GelApp *self)
{
	gel_list_deep_free(self->priv->paths, g_free);
	self->priv->paths = build_paths();
	return g_list_copy(self->priv->paths);
}

// --
// Getting from know plugins
// --
GelPlugin*
gel_app_get_plugin(GelApp *self, gchar *pathname, gchar *name)
{
	gchar *symbol = g_strconcat(name, "_plugin", NULL);

	GList *iter = self->priv->plugins;
	while (iter != NULL)
	{
		if (gel_plugin_matches(GEL_PLUGIN(iter->data), pathname, symbol))
		{
			g_free(symbol);
			return GEL_PLUGIN(iter->data);
		}
		iter = iter->next;
	}
	
	g_free(symbol);
	return NULL;
}

GelPlugin*
gel_app_get_plugin_by_pathname(GelApp *self, gchar *pathname)
{
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name = g_path_get_basename(dirname);
	g_free(dirname);
	GelPlugin *plugin = gel_app_get_plugin(self, pathname, name);
	g_free(name);

	return plugin;
}

GelPlugin*
gel_app_get_plugin_by_name(GelApp *self, gchar *name)
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


// --
// Querying
// --
GelPlugin *
gel_app_query_plugin(GelApp *self, gchar *pathname, gchar *name, GError **error)
{
	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin(self, pathname, name);
	if (plugin != NULL)
	{
		gel_plugin_ref(plugin);
		return plugin;
	}

	gchar *symbol = g_strconcat(name, "_plugin", NULL);
	if ((plugin = gel_plugin_new(self, pathname, symbol, error)) != NULL)
		self->priv->plugins = g_list_append(self->priv->plugins, plugin);
	g_free(symbol);

	return plugin;
}

GelPlugin *
gel_app_query_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{
	if (!self && !pathname) return NULL;

	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin_by_pathname(self, pathname);
	if (plugin != NULL)
	{
		gel_plugin_ref(plugin);
		return plugin;
	}

	// Create new plugin
	gchar *symbol = symbol_from_pathname(pathname);
	if ((plugin = gel_plugin_new(self, pathname, symbol, error)) != NULL)
		self->priv->plugins = g_list_append(self->priv->plugins, plugin);
	g_free(symbol);

	if (plugin == NULL)
	{
		g_set_error(error, gel_app_quark(), GEL_APP_PLUGIN_NOT_FOUND, N_("Plugin not found"));
		return NULL;
	}

	return plugin;
}

GelPlugin *
gel_app_query_plugin_by_name(GelApp *self, gchar *name, GError **error)
{
	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin_by_name(self, name);
	if (plugin != NULL)
	{
		gel_plugin_ref(plugin);
		return plugin;
	}

	// Search into paths for matching plugin
	GList *paths = gel_app_query_paths(self);
	GList *iter = paths;
	while (iter)
	{
		gchar *pathname = g_module_build_path((gchar *) iter->data, name);
		if ((plugin = gel_plugin_new(self, pathname, name, NULL)) != NULL)
		{
			self->priv->plugins = g_list_append(self->priv->plugins, plugin);
			g_free(pathname);
			g_list_free(paths);
			return plugin;
		}
		iter = iter->next;
	}
	g_list_free(paths);

	// Build-in
	gchar *symbol = g_strconcat(name, "_plugin", NULL);
	if ((plugin = gel_plugin_new(self, NULL, symbol, NULL)) != NULL)
		self->priv->plugins = g_list_append(self->priv->plugins, plugin);
	g_free(symbol);

	// Not found
	if (plugin == NULL)
		g_set_error(error, gel_app_quark(), GEL_APP_PLUGIN_NOT_FOUND, N_("Plugin not found"));
	return plugin;
}

GList *
gel_app_query_plugins(GelApp *self)
{
    GList *paths = gel_app_query_paths(self);
	GList *iter  = paths;
	GList *ret = NULL;

	while (iter)
	{
		GList *child, *children;
		child = children = gel_dir_read(iter->data, TRUE, NULL);
		while (child)
		{
			gchar *plugin_name = g_path_get_basename(child->data);
			gchar *module_path = g_module_build_path(child->data, plugin_name);
			g_free(plugin_name);

			GelPlugin *plugin = gel_app_query_plugin_by_pathname(self, module_path, NULL);
			g_free(module_path);

			if (plugin)
				ret = g_list_prepend(ret, plugin);

			child = child->next;
		}
		gel_list_deep_free(children, g_free);

		iter = iter->next;
	}
	g_list_free(paths);

    return ret;
}

// --
// Load plugins
// --

GelPlugin *
gel_app_load_plugin(GelApp *self, gchar *pathname, gchar *name, GError **error)
{
	GelPlugin *plugin = gel_app_query_plugin(self, pathname, name, error);
	if (plugin == NULL)
		return NULL;
	
	// Plugin is already ref'ed since we get if using _query style function

	if (!gel_plugin_init(plugin, error))
		return NULL;

	return plugin;
}


GelPlugin *
gel_app_load_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{
	// Just build symbol and call gel_app_load_plugin
	gchar *symbol = symbol_from_pathname(pathname);
	GelPlugin *ret = gel_app_load_plugin(self, pathname, symbol, error);
	g_free(symbol);
	return ret;
}

GelPlugin *
gel_app_load_plugin_by_name(GelApp *self, gchar *name, GError **error)
{
	GelPlugin *ret = NULL;

	// Search in paths for a matching filename
	GList *paths = gel_app_query_paths(self);
	GList *iter = paths;
	while (iter)
	{
		gchar *pathname = g_module_build_path((gchar *) iter->data, name);
		if ((ret = gel_app_load_plugin(self, pathname, name, NULL)) != NULL)
		{
			g_free(pathname);
			return ret;
		}
		iter = iter->next;
	}
	g_list_free(paths);

	// Plugin not found, try using build-in feature
	return gel_app_load_plugin(self, NULL, name, error);
}

// --
// Unload plugins, _by_name and _by_pathname are provided via macros
// --

gboolean
gel_app_unload_plugin(GelApp *self, GelPlugin *plugin, GError **error)
{
	if (plugin == NULL)
		return FALSE;

	if (!gel_plugin_fini(plugin, error))
		return FALSE;

	gel_plugin_unref(plugin);
	return TRUE;
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

void
gel_app_emit_signal(GelApp *self, guint id, GelPlugin *plugin)
{
	if (!is_owned(self,plugin))
		return;
	g_signal_emit(self, gel_app_signals[id], 0, plugin);
}

void
gel_app_emit_load(GelApp *self, GelPlugin *plugin)
{
	gel_app_emit_signal(self, PLUGIN_LOAD, plugin);
}

void
gel_app_emit_unload(GelApp *self, GelPlugin *plugin)
{
	gel_app_emit_signal(self, PLUGIN_UNLOAD, plugin);
}

void
gel_app_emit_init(GelApp *self, GelPlugin *plugin)
{
	gel_app_emit_signal(self, PLUGIN_INIT, plugin);
}

void
gel_app_emit_fini(GelApp *self, GelPlugin *plugin)
{
	gel_app_emit_signal(self, PLUGIN_FINI, plugin);
}

void
gel_app_delete_plugin(GelApp *self, GelPlugin *plugin)
{
	if (gel_plugin_is_enabled(plugin) || (gel_plugin_get_usage(plugin) > 0))
	{
		g_warning("Invalid deletion");
		return;
	}

	self->priv->plugins = g_list_remove(self->priv->plugins, plugin);
}

// --
// Statics
// --


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

