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

#define GEL_DOMAIN "Gel::Plugin"
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
	((gel_plugin_get_app(plugin) == self) && (g_hash_table_lookup(self->priv->lookup, gel_plugin_stringify(plugin)) != NULL))

struct _GelAppPrivate {
	GelAppDisposeFunc dispose_func;
	gpointer  dispose_data;

	GHashTable *shared;  // Shared memory
	GList      *paths;   // Paths to search plugins

	GHashTable *lookup;  // Fast lookup table
};

enum {
	PLUGIN_INIT,
	PLUGIN_FINI,
	PLUGIN_LOAD,
	PLUGIN_UNLOAD,

	LAST_SIGNAL
};
static guint gel_app_signals[LAST_SIGNAL] = { 0 };

static gchar *
list_loaded_plugins(GelApp *self)
{
	GList *l = NULL;
	GList *plugins = g_hash_table_get_values(self->priv->lookup);
	GList *iter = plugins;
	while (iter)
	{
		l = g_list_prepend(l, (gpointer) gel_plugin_stringify((GelPlugin *) iter->data));
		iter = iter->next;
	}
	g_list_free(plugins);

	gchar *ret = gel_list_join("\n", l);
	g_list_free(l);

	return ret;
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
		GList *plugins = g_hash_table_get_values(self->priv->lookup);
		if (plugins)
		{
			g_warning(N_("%d plugins still loadeds, will be leaked. Use a dispose func (gel_app_set_dispose_func) to unload them at exit"),
				g_list_length(plugins));
			gchar *tmp = list_loaded_plugins(self);
			g_warning("%s", tmp);
			g_free(tmp);
			g_list_free(plugins);
			g_hash_table_destroy(self->priv->lookup);
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
}

static void
gel_app_init (GelApp *self)
{
	GelAppPriv *priv = self->priv = g_new0(GelAppPriv, 1);

	priv->dispose_func = priv->dispose_data = NULL;
	priv->paths  = NULL;
	priv->shared = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	priv->lookup = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

GelApp*
gel_app_new (void)
{
	return g_object_new (GEL_TYPE_APP, NULL);
}

void
gel_app_set_dispose_callback(GelApp *self, GelAppDisposeFunc callback, gpointer user_data)
{
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
	if (self->priv->paths == NULL)
		self->priv->paths = build_paths();
	return g_list_copy(self->priv->paths);
}

// --
// Getting from know plugins
// --
GelPlugin*
gel_app_get_plugin(GelApp *self, gchar *pathname, gchar *name)
{
	// DONT remove this comment
	// g_return_val_if_fail(pathname != NULL, NULL); // pathname can be NULL,
	g_return_val_if_fail(name != NULL, NULL);

	gchar *pstr = gel_plugin_util_stringify(pathname, name);
	GelPlugin *ret = g_hash_table_lookup(self->priv->lookup, pstr);
	g_free(pstr);

	return ret;
}

GelPlugin*
gel_app_get_plugin_by_pathname(GelApp *self, gchar *pathname)
{
	g_return_val_if_fail(pathname != NULL, NULL);

	gchar *pstr = gel_plugin_util_stringify_for_pathname(pathname);
	GelPlugin *ret =  g_hash_table_lookup(self->priv->lookup, pstr);
	g_free(pstr);

	return ret;
}

GelPlugin*
gel_app_get_plugin_by_name(GelApp *self, gchar *name)
{
	g_return_val_if_fail(name != NULL, NULL);

	GelPlugin *ret = NULL;
	GList *paths = gel_app_query_paths(self);
	paths = g_list_prepend(paths, NULL);

	GList *iter = paths;
	while (iter)
	{
		if ((ret = gel_app_get_plugin(self, (gchar *) iter->data, name)) != NULL)
			break;
		iter = iter->next;
	}
	g_list_free(paths);

	return ret;
}

GList *
gel_app_get_plugins(GelApp *self)
{
	return g_hash_table_get_values(self->priv->lookup);
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
		return plugin;

	gchar *symbol = g_strconcat(name, "_plugin", NULL);
	plugin = gel_plugin_new(self, pathname, symbol, error);
	g_free(symbol);

	return plugin;
}

GelPlugin *
gel_app_query_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{	
	if (!self || !pathname)
	{
		g_set_error(error, gel_app_quark(), GEL_APP_ERROR_GENERIC, N_("Invalid arguments"));
		return NULL;
	}

	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin_by_pathname(self, pathname);
	if (plugin != NULL)
		return plugin;

	// Create new plugin
	gchar *symbol = symbol_from_pathname(pathname);
	plugin = gel_plugin_new(self, pathname, symbol, error);
	g_free(symbol);

	return plugin;
}

GelPlugin *
gel_app_query_plugin_by_name(GelApp *self, gchar *name, GError **error)
{
	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin_by_name(self, name);
	if (plugin != NULL)
		return plugin;

	// Search into paths for matching plugin
	GList *paths = gel_app_query_paths(self);
	GList *iter = paths;
	while (iter)
	{
		gchar *pathname = g_module_build_path((gchar *) iter->data, name);
		if ((plugin = gel_plugin_new(self, pathname, name, NULL)) != NULL)
		{
			g_free(pathname);
			g_list_free(paths);
			return plugin;
		}
		iter = iter->next;
	}
	g_list_free(paths);

	// Build-in
	gchar *symbol = g_strconcat(name, "_plugin", NULL);
	if ((plugin = gel_plugin_new(self, NULL, symbol, NULL)) == NULL)
		g_set_error(error, gel_app_quark(), GEL_APP_PLUGIN_NOT_FOUND, N_("Plugin not found"));
	g_free(symbol);

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
		gel_warn("Inspecting %s", iter->data);
		child = children = gel_dir_read(iter->data, TRUE, NULL);
		while (child)
		{
			gchar *plugin_name = g_path_get_basename(child->data);
			gchar *module_path = g_module_build_path(child->data, plugin_name);
			g_free(plugin_name);
			gel_warn("Querying for plugin %s", module_path);

			GError *err = NULL;
			GelPlugin *plugin = gel_app_query_plugin_by_pathname(self, module_path, &err);

			if (plugin)
				ret = g_list_prepend(ret, plugin);
			else
			{
				gel_error("Cannot load %s: %s", module_path, err->message);
				g_error_free(err);
			}
			g_free(module_path);

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
#if 0
static gboolean
unload_n_deps(GelApp *app, GelPlugin *plugin, gint n, GError **error)
{
	if (plugin->depends == NULL)
		return TRUE;

	gchar **deps = g_strsplit(plugin->depends, ",", 0);

	if (n < 0)
		n = g_strv_length(deps) - 1;

	while (n >= 0)
	{
		GelPlugin *p = gel_app_get_plugin_by_name(app, deps[n]);
		if (p)
		{
			gel_plugin_remove_reference(p, plugin);
		/*
			if (!gel_plugin_is_in_use(p))
			{
				if (!gel_plugin_fini(p, NULL))
					gel_warn("EH?!");
			}
			*/
		}
		n--;
	}
	g_free(deps);
	return TRUE;
}

gboolean
gel_app_unload_plugin_dependences(GelApp *app, GelPlugin *plugin, GError **error)
{
	return unload_n_deps(app, plugin, -1, error);
}
#endif

gboolean
gel_app_load_plugin_dependences(GelApp *app, GelPlugin *plugin, GError **error)
{
	if (plugin->depends == NULL)
		return TRUE;

	gchar **deps = g_strsplit(plugin->depends, ",", 0);
	gint i = 0;
	while (deps[i])
	{
		GError *err = NULL;
		GelPlugin *p = NULL;
		if ((p = gel_app_load_plugin_by_name(app, deps[i], &err)) == NULL)
		{
			g_set_error(error, gel_app_quark(), GEL_APP_PLUGIN_DEP_NOT_FOUND,
				N_("Dependecy %s for %s not found"), deps[i], gel_plugin_stringify(plugin));
			g_error_free(err);
			break;
		}
		gel_plugin_add_reference(p, plugin);
		i++;
	}
	g_free(deps);

	return (deps[i] == NULL);
#if 0
	if (deps[i] == NULL)
		return TRUE;
	else
	{
		if (i > 0)
		{
			// gel_warn("Calling rollback with n = %d", i - 1);
			gel_warn("Rollback started from %d", i - 1);
			unload_n_deps(app, plugin, i - 1, NULL);
		}
		return FALSE;
	}
#endif
}

GelPlugin *
gel_app_load_plugin(GelApp *self, gchar *pathname, gchar *name, GError **error)
{
	// Check if another plugin with the same name is already loaded
	GelPlugin *plugin = gel_app_get_plugin_by_name(self,name);
	if (plugin && gel_plugin_is_enabled(plugin))
		/*
		g_set_error(error, gel_app_quark(), GEL_APP_ERROR_PLUGIN_ALREADY_LOADED_BY_NAME,
			N_("Another plugin (%s) with the same name (%s) is loaded and initialized"), gel_plugin_get_pathname(plugin), name);
		*/
		return plugin;
	
	if (!plugin && ((plugin = gel_app_query_plugin(self, pathname, name, error)) == NULL))
		return NULL;

	// PLugin is already enabled, continue
	if (gel_plugin_is_enabled(plugin))
	{
		// gel_warn("Plugin %s already enabled, its ok", gel_plugin_stringify(plugin));
		return plugin;
	}

	// Before init we need to load other deps
	if (!gel_app_load_plugin_dependences(self, plugin, error))
	{
		// gel_warn("Cannot load dependences for %s: %s", gel_plugin_stringify(plugin), (*error)->message);
		return NULL;
	}

	// Call init hook
	if (!gel_plugin_init(plugin, error))
	{
		// gel_warn("Cannot init %s: %s (unload deps)", gel_plugin_stringify(plugin), (*error)->message);
		return NULL;
	}

	return plugin;
}

GelPlugin *
gel_app_load_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{
	// Just build name and call gel_app_load_plugin
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name = g_path_get_basename(dirname);
	g_free(dirname);
	GelPlugin *ret = gel_app_load_plugin(self, pathname, name, error);
	g_free(name);
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
		gchar *parent = g_build_filename((gchar *) iter->data, name, NULL);
		gchar *pathname = g_module_build_path(parent, name);
		g_free(parent);

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
	g_return_val_if_fail(plugin != NULL, FALSE);
	GError *err = NULL;

	// Check if any other plugin references this plugin
	if (gel_plugin_is_in_use(plugin))
	{
		g_set_error(error, gel_app_quark(), GEL_APP_ERROR_PLUGIN_IN_USE,
			N_("Cannot unload plugin %s, its used by %d plugins"),
			gel_plugin_stringify(plugin), gel_plugin_get_usage(plugin));
		gel_warn(N_("Cannot unload plugin %s, its used by %d plugins"),
			gel_plugin_stringify(plugin), gel_plugin_get_usage(plugin));
		return FALSE;
	}

	// Disable in case it was enabled
	if (gel_plugin_is_enabled(plugin) && !gel_plugin_fini(plugin, &err))
	{
		gel_warn("Cannot finalize plugin %s: %s", gel_plugin_stringify(plugin), err->message);
		g_propagate_error(error, err);
		return FALSE;
	}

#if 0
	// Now remove references from this plugin
	if (!gel_app_unload_plugin_dependences(self, plugin, &err))
	{
		gel_warn("Error while unloading deps for %s", gel_plugin_stringify(plugin));
		g_propagate_error(error, err);
		return FALSE;
	}
#endif

	if (gel_plugin_is_locked(plugin))
		return TRUE;

	if (!gel_plugin_free(plugin, &err))
	{
		gel_warn("Cannot free plugin after all: %s", err->message);
		g_propagate_error(error, err);
		return FALSE;
	}

	return TRUE;
}

void
gel_app_purge(GelApp *app)
{
	GList *plugins = g_hash_table_get_values(app->priv->lookup);
	GList *iter = plugins;
	while (iter)
	{
		GelPlugin *plugin = GEL_PLUGIN(iter->data);
		GError *err = NULL;

		if (!gel_plugin_is_enabled(plugin) && !gel_plugin_is_in_use(plugin))
		{
			gchar *pstr = g_strdup(gel_plugin_stringify(plugin));
			if (gel_plugin_free(plugin, &err))
			{
				gel_warn("PURGE %s", pstr);
			}
			else
			{
				gel_error("Cannot free plugin %s: %s", gel_plugin_stringify(plugin), err->message);
				g_error_free(err);
			}
			g_free(pstr);
		}

		iter = iter->next;
	}
	g_list_free(plugins);
}

// --
// Shared management
// --
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
/*
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
*/
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
gel_app_add_plugin(GelApp *self, GelPlugin *plugin)
{
	g_return_if_fail(plugin != NULL);
	const gchar *pstr = gel_plugin_stringify(plugin);
	g_return_if_fail(g_hash_table_lookup(self->priv->lookup, pstr) == NULL);
	
	g_hash_table_insert(self->priv->lookup, g_strdup(pstr), plugin);
	gel_warn("Added plugin %s", gel_plugin_stringify(plugin));
	gel_app_emit_signal(self, PLUGIN_LOAD, plugin);
}

void
gel_app_remove_plugin(GelApp *self, GelPlugin *plugin)
{
	g_return_if_fail(plugin != NULL);
	const gchar *pstr = gel_plugin_stringify(plugin);
	g_return_if_fail(g_hash_table_lookup(self->priv->lookup, pstr) != NULL);
	g_return_if_fail(gel_plugin_is_in_use(plugin) == FALSE);
	g_return_if_fail(gel_plugin_is_enabled(plugin) == FALSE);

	g_hash_table_remove(self->priv->lookup, pstr);
	gel_warn("Removed plugin %s", gel_plugin_stringify(plugin));
	gel_app_emit_signal(self, PLUGIN_UNLOAD, plugin);
}

// --
// Statics
// --


static GList*
build_paths(void)
{
	GList *ret = NULL;

	const gchar *libdir = gel_get_package_lib_dir();
	if (libdir)
		ret = g_list_prepend(ret, g_strdup(libdir));

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

