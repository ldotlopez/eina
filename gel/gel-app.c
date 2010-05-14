/*
 * gel/gel-app.c
 *
 * Copyright (C) 2004-2010 Eina
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
#include <gel/gel-app-marshallers.h>

G_DEFINE_TYPE (GelApp, gel_app, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_TYPE_APP, GelAppPrivate))

GEL_DEFINE_QUARK_FUNC(app)

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

		// Unload plugins
		GList *plugins = gel_app_get_plugins(self);
		GList *l = plugins;

		g_list_foreach(plugins, (GFunc)  gel_plugin_remove_lock, NULL);
		while (l)
		{
			GelPlugin *plugin = GEL_PLUGIN(l->data);

			if (!gel_plugin_is_in_use(plugin))
			{
				l = plugins = g_list_remove(plugins, plugin);
				gel_app_unload_plugin(self, plugin, NULL);
			}
			else
				l = l->next;
		}
		g_list_free(plugins);

		plugins = gel_app_get_plugins(self);
		if (plugins)
			gel_warn(N_("%d plugins are enabled at exit. Use a dispose func to unload them."), g_list_length(plugins));
		GList *iter = plugins;
		while (iter)
		{
			GelPlugin *plugin = GEL_PLUGIN(iter->data);
			gchar *rdeps_str = gel_plugin_stringify_dependants(plugin);
			gel_warn(N_("  %s (usage:%d enabled:%d locked:%d %s)"),
				gel_plugin_stringify(plugin),
				gel_plugin_get_usage(plugin),
				gel_plugin_is_enabled(plugin),
				gel_plugin_is_locked(plugin),
				rdeps_str);
			g_free(rdeps_str);
			iter = iter->next;
		}
		g_list_free(plugins);

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

GList *
gel_app_query_paths(GelApp *self)
{
	if (self->priv->paths == NULL)
		self->priv->paths = build_paths();
	return g_list_copy(self->priv->paths);
}

// --
// Getting from know plugins (full, by pathname, by name)
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

	GList *plugins, *iter;
	GelPlugin *ret = NULL;
	plugins = iter = gel_app_get_plugins(self);
	while (iter)
	{
		if (g_str_equal(GEL_PLUGIN(iter->data)->name, name))
		{
			ret = GEL_PLUGIN(iter->data);
			break;
		}
		iter = iter->next;
	}
	g_list_free(plugins);

	return ret;
}

GList *
gel_app_get_plugins(GelApp *self)
{
	return g_hash_table_get_values(self->priv->lookup);
}

// --
// Querying for plugins (loaded or not) and full, by pathname, by name
// --
GelPlugin *
gel_app_query_plugin(GelApp *self, gchar *pathname, gchar *name, GError **error)
{
	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin(self, pathname, name);
	if (plugin != NULL)
		return plugin;

	gchar *symbol = gel_plugin_util_symbol_from_name(name);
	plugin = gel_plugin_new(self, pathname, symbol, error);
	g_free(symbol);

	return plugin;
}

GelPlugin *
gel_app_query_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{
	if (!self || !pathname)
	{
		g_set_error(error, app_quark(), GEL_APP_ERROR_GENERIC, N_("Invalid arguments"));
		return NULL;
	}

	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin_by_pathname(self, pathname);
	if (plugin != NULL)
		return plugin;

	// Create new plugin
	gchar *symbol = gel_plugin_util_symbol_from_pathname(pathname);
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
		g_set_error(error, app_quark(), GEL_APP_PLUGIN_NOT_FOUND, N_("Plugin not found"));
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
		gel_warn("[?] %s", iter->data);
		child = children = gel_dir_read(iter->data, TRUE, NULL);
		while (child)
		{
			gchar *plugin_name = g_path_get_basename(child->data);
			gchar *module_path = g_module_build_path(child->data, plugin_name);
			g_free(plugin_name);

			GError *err = NULL;
			GelPlugin *plugin = gel_app_query_plugin_by_pathname(self, module_path, &err);

			if (plugin)
				ret = g_list_prepend(ret, plugin);
			else
			{
				gel_error(N_("Cannot load %s: %s"), module_path, err->message);
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
// Load and unload plugins
// --
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
			g_set_error(error, app_quark(), GEL_APP_PLUGIN_DEP_NOT_FOUND,
				N_("Dependecy %s for %s not found"), deps[i], gel_plugin_stringify(plugin));
			g_error_free(err);
			break;
		}
		gel_plugin_add_reference(p, plugin);
		i++;
	}
	g_free(deps);

	return (deps[i] == NULL);
}

GelPlugin *
gel_app_load_plugin(GelApp *self, gchar *pathname, gchar *name, GError **error)
{
	if ((self == NULL) || (name == NULL))
	{
		g_set_error(error, app_quark(), GEL_APP_ERROR_INVALID_ARGUMENTS,
			N_("Invalid arguments, set breakpoint on %s to debug"), name);
		return NULL;
	}

	// Check if another plugin with the same name is already loaded
	// Set some pointers to posible matchmes.
	GelPlugin *exact_plugin = NULL, *fuzzy_plugin = NULL;
	GList *plugins, *iter;
	plugins = iter = gel_app_get_plugins(self);
	while (iter)
	{
		GelPlugin *p = GEL_PLUGIN(iter->data);

		// Invalid data
		if (p->name == NULL)
		{
			gel_warn(N_("Plugin %p has NULL name"), p);
			iter = iter->next;
			continue;
		}

		// not match
		if (!g_str_equal(p->name, name))
		{
			iter = iter->next;
			continue;
		}

		// Name matches at this point
		if (
			((pathname == NULL) && (gel_plugin_get_pathname(p) == NULL)) || // Both have NULL path
			((pathname != NULL) && (gel_plugin_get_pathname(p) != NULL) && g_str_equal(gel_plugin_get_pathname(p), pathname)) // path match
		)
			exact_plugin = p;
		else
			fuzzy_plugin = p;

		iter = iter->next;
	}
	g_list_free(plugins);

	// Exact match found and enabled, nothing to do
	if (exact_plugin && gel_plugin_is_enabled(exact_plugin))
		return exact_plugin;

	// Fuzzy plugin (matches name) found, if its enabled return it. Two enabled
	// plugins cannot have the same name, warn about this.
	if (fuzzy_plugin && gel_plugin_is_enabled(fuzzy_plugin))
	{
		gel_warn("[~] %s", gel_plugin_stringify(fuzzy_plugin));
		return fuzzy_plugin;
	}

	// Ok, maybe we have nothing and we can get nothing :(
	if (!exact_plugin && ((exact_plugin = gel_app_query_plugin(self, pathname, name, error)) == NULL))
		return NULL;

	// Try to enable this exact_match and all the stuff

	// Before init we need to load other deps
	if (!gel_app_load_plugin_dependences(self, exact_plugin, error))
	{
		// gel_warn("Cannot load dependences for %s: %s", gel_plugin_stringify(plugin), (*error)->message);
		return NULL;
	}

	// Call init hook
	if (!gel_plugin_init(exact_plugin, error))
	{
		gel_error(N_("Init for '%s' failed: %s"), gel_plugin_stringify(exact_plugin), error ? (*error)->message : N_("No error"));
		if (gel_plugin_is_locked(exact_plugin))
		{
			gel_error(N_("Failed plugin '%s' is locked, unload aborted."), gel_plugin_stringify(exact_plugin));
			return NULL;
		}

		GError *err2 = NULL;
		if (!gel_app_unload_plugin(self, exact_plugin, &err2))
		{
			gel_error(N_("Error unloading failed plugin '%s': %s. Problems ahead"), gel_plugin_stringify(exact_plugin), err2->message);
			g_error_free(err2);
			return NULL;
		}
		return NULL;
	}

	return exact_plugin;
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
		g_set_error(error, app_quark(), GEL_APP_ERROR_PLUGIN_IN_USE,
			N_("Cannot unload plugin %s, its used by %d plugins"),
			gel_plugin_stringify(plugin), gel_plugin_get_usage(plugin));
		gel_error(N_("Cannot unload plugin %s, its used by %d plugins"),
			gel_plugin_stringify(plugin), gel_plugin_get_usage(plugin));
		gchar *tmp = gel_plugin_stringify_dependants(plugin);
		gel_error(tmp);
		g_free(tmp);
		return FALSE;
	}

	// Disable in case it was enabled
	// gboolean was_enabled = gel_plugin_is_enabled(plugin);
	if (gel_plugin_is_enabled(plugin) && !gel_plugin_fini(plugin, &err))
	{
		gel_warn(N_("Cannot finalize plugin %s: %s"), gel_plugin_stringify(plugin), err->message);
		g_propagate_error(error, err);
		return FALSE;
	}

	// Remove deps from dependencies if plugin is active
	// if ((plugin->depends && was_enabled))

	// Note: 2010-01-20
	// references are loaded before plugin was initialized, so we have to
	// remove them whatever it has enabled or not. This change hasn't be
	// sufficient tested, so I leave this comment here (xuzo@cuarentaydos.com)
	if (plugin->depends)
	{
		gint i;
		gchar **deps = g_strsplit(plugin->depends, ",", 0);
		for (i = 0; deps && (deps[i] != NULL); i++)
		{
			GelPlugin *p = gel_app_get_plugin_by_name(self, deps[i]);
			if (p == NULL)
			{
				gel_warn(N_("Dependency %s for plugin %s not found"), deps[i], gel_plugin_stringify(plugin));
				continue;
			}
			gel_plugin_remove_reference(p, plugin);
		}
	}

	if (gel_plugin_is_locked(plugin))
		return TRUE;

	if (!gel_plugin_free(plugin, &err))
	{
		gel_warn(N_("Cannot free plugin after all: %s"), err->message);
		g_propagate_error(error, err);
		return FALSE;
	}

	return TRUE;
}

// --
// Automatically purges all plugins which are neither initialized or locked
// Usefull to cleanup a GelApp after a query_plugins call
// --
void
gel_app_purge(GelApp *app)
{
	GList *plugins, *iter;
	plugins = iter = gel_app_get_plugins(app);
	gel_warn("[·] Purge");
	while (iter)
	{
		GelPlugin *plugin = GEL_PLUGIN(iter->data);
		GError *err = NULL;

		if (gel_plugin_is_enabled(plugin) || gel_plugin_is_in_use(plugin) || gel_plugin_is_locked(plugin))
		{
			iter = iter->next;
			continue;
		}

		gchar *pstr = g_strdup(gel_plugin_stringify(plugin));
		if (!gel_plugin_free(plugin, &err))
		{
			gel_error(N_("Cannot free plugin %s: %s"), gel_plugin_stringify(plugin), err->message);
			g_error_free(err);
		}

		g_free(pstr);

		iter = iter->next;
	}
	g_list_free(plugins);
}

// --
// Shared memory management
// --
gboolean gel_app_shared_register(GelApp *self, gchar *name, gsize size)
{
	g_return_val_if_fail(gel_app_shared_get(self, name) == NULL, FALSE);
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
	g_return_val_if_fail(gel_app_shared_get(self, name) == NULL, FALSE);
	g_hash_table_insert(self->priv->shared, g_strdup(name), data);

	return TRUE;
}

gpointer gel_app_shared_get
(GelApp *self, gchar *name)
{
	return g_hash_table_lookup(self->priv->shared, name);
}

// --
// Proxies for "init" and "fini" signals. To be called from GelPlugin module
// Only these functions can emit those signals.
// --
void
gel_app_emit_init(GelApp *self, GelPlugin *plugin)
{
	g_signal_emit(self, gel_app_signals[PLUGIN_INIT], 0, plugin);
}

void
gel_app_emit_fini(GelApp *self, GelPlugin *plugin)
{
	g_signal_emit(self, gel_app_signals[PLUGIN_FINI], 0, plugin);
}

// --
// Add and remove plugins from app registry, called from GelPlugin module
// Only these functions are allowed to access self->priv->lookup table and
// emit "load" and "unload" signals
// --
void
gel_app_add_plugin(GelApp *self, GelPlugin *plugin)
{
	g_return_if_fail(plugin != NULL);
	const gchar *pstr = gel_plugin_stringify(plugin);
	g_return_if_fail(g_hash_table_lookup(self->priv->lookup, pstr) == NULL);

	g_hash_table_insert(self->priv->lookup, g_strdup(pstr), plugin);
	gel_warn("[+] %s", gel_plugin_stringify(plugin));
	g_signal_emit(self, gel_app_signals[PLUGIN_LOAD], 0, plugin);
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
	gel_warn("[-] %s", gel_plugin_stringify(plugin));
	g_signal_emit(self, gel_app_signals[PLUGIN_UNLOAD], 0, plugin);
}

// --
// Statics
// --
static GList*
build_paths(void)
{
	GList *ret = NULL;

	// 1. Add multiple ${PRGNAME}_PLUGINS_PATH
	// 2. If not enviroment variable
	//   2.1 Add user's dir
	//   2.2 Add system dir

	const gchar *envdir = gel_resource_type_get_env(GEL_RESOURCE_SHARED);
	if (envdir)
	{
		gchar **paths = g_strsplit(envdir, ":", -1);
		gint i;
		for (i = 0; paths[i] != NULL; i++)
			if (paths[i][0])
				ret = g_list_prepend(ret, g_strdup(paths[i]));

		g_strfreev(paths);
		return g_list_reverse(ret);
	}

	gchar *userdir = gel_resource_type_get_user_dir(GEL_RESOURCE_SHARED);
	if (userdir)
		ret = g_list_prepend(ret, userdir);
	gchar *systemdir = gel_resource_type_get_system_dir(GEL_RESOURCE_SHARED);
	if (systemdir)
		ret = g_list_prepend(ret, systemdir);

	return g_list_reverse(ret);
}

