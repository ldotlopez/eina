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
	GQueue     *stack;
};

enum {
	PLUGIN_INIT,
	PLUGIN_FINI,

	LAST_SIGNAL
};
static guint gel_app_signals[LAST_SIGNAL] = { 0 };

static void
gel_app_dispose (GObject *object)
{
	GelApp *self = GEL_APP(object);

	if (self->priv == NULL)
	{
		G_OBJECT_CLASS (gel_app_parent_class)->dispose (object);
		return;
	}

	// Call dispose before destroy us
	if (self->priv->dispose_func)
	{   
		self->priv->dispose_func(self, self->priv->dispose_data);
		self->priv->dispose_func = NULL;
	}

	GelPlugin *plugin = NULL;
	while ((plugin = g_queue_peek_tail(self->priv->stack)))
	{
		GError *error = NULL;
		const GelPluginInfo *info = gel_plugin_get_info(plugin);

		gel_warn("Must unload %s", info->name);
		if (!gel_app_unload_plugin(self, plugin, &error))
		{
			gel_warn(N_("Plugin '%s' cannot be unloaded: %s"),
				info->name, error ? error->message : N_("No error"));
			g_error_free(error);
			break;
		}
	}

	// Manage queue
	if (g_queue_get_length(self->priv->stack))
	{
		gel_warn(N_("Plugin stack has remaining items, they will be leaked"));
		g_queue_clear(self->priv->stack);
	}
	gel_free_and_invalidate(self->priv->stack, NULL, g_queue_free);

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
}

static void
gel_app_init (GelApp *self)
{
	GelAppPriv *priv = self->priv = g_new0(GelAppPriv, 1);

	priv->dispose_func = priv->dispose_data = NULL;
	priv->paths  = NULL;
	priv->shared = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	priv->lookup = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	priv->stack  = g_queue_new();
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

// XXX: Needs API change, list must be deep copied
GList *
gel_app_query_paths(GelApp *self)
{
	if (self->priv->paths == NULL)
		self->priv->paths = build_paths();
	
	GList *ret = NULL;
	GList *iter = self->priv->paths;
	while (iter)
	{
		ret = g_list_prepend(ret, g_strdup(iter->data));
		iter = iter->next;
	}
	return g_list_reverse(ret);
}

// --
// Getting from know plugins (full, by pathname, by name)
// --
GelPlugin*
gel_app_get_plugin(GelApp *self, gchar *pathname, gchar *name)
{
	gel_warn(__FUNCTION__);
#if 0
	// DONT remove this comment
	// g_return_val_if_fail(pathname != NULL, NULL); // pathname can be NULL,
	g_return_val_if_fail(name != NULL, NULL);

	gchar *pstr = gel_plugin_util_stringify(pathname, name);
	GelPlugin *ret = g_hash_table_lookup(self->priv->lookup, pstr);
	g_free(pstr);

	return ret;
#endif
	return NULL;
}

GelPlugin*
gel_app_get_plugin_by_pathname(GelApp *self, gchar *pathname)
{
	gel_warn(__FUNCTION__);
#if 0
	g_return_val_if_fail(pathname != NULL, NULL);

	gchar *pstr = gel_plugin_util_stringify_for_pathname(pathname);
	GelPlugin *ret =  g_hash_table_lookup(self->priv->lookup, pstr);
	g_free(pstr);

	return ret;
#endif
	return NULL;
}

GelPlugin*
gel_app_get_plugin_by_name(GelApp *self, gchar *name)
{
	g_return_val_if_fail(GEL_IS_APP(self), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	gel_warn(__FUNCTION__);
	return g_hash_table_lookup(self->priv->lookup, name);

#if 0
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
#endif
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
	gel_warn(__FUNCTION__);
	return NULL;
#if 0
	// Search in know plugins
	GelPlugin *plugin = gel_app_get_plugin(self, pathname, name);
	if (plugin != NULL)
		return plugin;

	gchar *symbol = gel_plugin_util_symbol_from_name(name);
	plugin = gel_plugin_new(self, pathname, symbol, error);
	g_free(symbol);

	return plugin;
#endif
}

GelPlugin *
gel_app_query_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{
	gel_warn(__FUNCTION__);
	return NULL;
#if 0
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
	gchar *symbol = gel_plugin_util_symbol_from_pathname(pathname);
	plugin = gel_plugin_new(self, pathname, symbol, error);
	g_free(symbol);

	return plugin;
#endif
}

GelPlugin *
gel_app_query_plugin_by_name(GelApp *self, gchar *name, GError **error)
{
	gel_warn(__FUNCTION__);
	return NULL;
#if 0
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
#endif
}

GList *
gel_app_query_plugins(GelApp *self)
{
	// gel_warn(__FUNCTION__);

	return NULL;

	GList *ret = NULL;
	GList *paths = gel_app_query_paths(self);
	GList *iter = paths;
	while (iter)
	{
		gchar *path = (gchar *) iter->data;

		GList *child, *children;
		// gel_warn(" Search plugins in %s", path);
		child = children = gel_dir_read(iter->data, TRUE, NULL);
		while (child)
		{
			// Build a path for the plugin info file
			gchar *basename = g_path_get_basename(child->data);
			gchar *infoname = g_strconcat(basename, ".ini", NULL);
			gchar *infopath = g_build_filename(path, basename, infoname, NULL);

			g_free(basename);
			g_free(infoname);

			// Maybe this infopath is already loaded as plugin

			// Read info file
			if (g_file_test(infopath, G_FILE_TEST_IS_REGULAR))
			{
				GelPluginInfo *pinfo = NULL;
				GError *error = NULL;
				if (!(pinfo = gel_plugin_info_new(infopath, NULL, &error)))
				{
					gel_error(N_("Cannot load info for '%s': %s"), infopath, error->message);
					g_error_free(error);
				}
				else
					ret = g_list_prepend(ret, pinfo);
			}
			g_free(infopath);

			child = child->next;
		}
		gel_list_deep_free(children, g_free);


		iter = iter->next;
	}
	gel_list_deep_free(paths, g_free);

	return g_list_reverse(ret);

#if 0
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
#endif
}

// --
// Load and unload plugins
// --
GelPlugin *
gel_app_load_plugin(GelApp *self, GelPluginInfo *info, GError **error)
{
 	g_return_val_if_fail(GEL_IS_APP(self), NULL);
 	g_return_val_if_fail(info, NULL);
 
 	gel_warn("BEGIN LOAD '%s'", info->name);
 
 	// Check if plugin is already loaded
 	GelPlugin *plugin = (GelPlugin *) g_hash_table_lookup(self->priv->lookup, info->name);
 	if (plugin)
  	{
 		gel_warn("[~] %s", info->name);
 		gel_warn("END LOAD '%s'", info->name);
 		return plugin;
 	}
 
 	// Load deps
 	if (info->depends && info->depends[0])
 	{
 		gel_warn("[?] '%s': %s", info->name, info->depends);
 		gchar **deps = g_strsplit(info->depends, ",", 0);
 		for (gint i = 0; deps[i] && deps[i][0]; i++)
 		{
 			GError *err = NULL;
			if (!gel_app_load_plugin_by_name(self, deps[i], &err))
			{
 				g_set_error(error, app_quark(), GEL_APP_MISSING_PLUGIN_DEPS,
 					N_("Failed to load plugin dependency '%s' for '%s': %s"), deps[i], info->name, err->message);
 				g_error_free(err);
 				g_strfreev(deps);
 				gel_warn("END LOAD '%s'", info->name);
 				return NULL;
 			}
 		}
 		g_strfreev(deps);
 	}

	// Load plugin itself
	if (!(plugin = gel_plugin_new(self, info, error)))
 	{
 		gel_warn("[!] %s", info->name);
 		gel_warn("END LOAD '%s'", info->name);
 
 		return NULL;
 	}
 
 	gel_warn("[+] %s", info->name);
 	gel_warn("END LOAD '%s'", info->name);
 
 	return plugin;
}

GelPlugin *
gel_app_load_plugin_by_pathname(GelApp *self, gchar *pathname, GError **error)
{
	gel_warn(__FUNCTION__);
	return NULL;
#if 0
	// Just build name and call gel_app_load_plugin
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name = g_path_get_basename(dirname);
	g_free(dirname);
	GelPlugin *ret = gel_app_load_plugin(self, pathname, name, error);
	g_free(name);
	return ret;
#endif
}

GelPlugin *
gel_app_load_plugin_by_name(GelApp *self, gchar *name, GError **error)
{
	// gel_warn("%s (%p, %s,%p)", __FUNCTION__, self, name, error );

	// 1. Query plugins and try to match name against one of them
	GList *plugins_info = gel_app_query_plugins(self);
	GList *iter = plugins_info;
	while (iter)
	{
		if (g_str_equal(((GelPluginInfo *) iter->data)->name, name))
			break;
		iter = iter->next;
	}

	GelPluginInfo *info = NULL;
	if (iter)
	{
		plugins_info = g_list_remove_link(plugins_info, iter);
		info = (GelPluginInfo *) iter->data;
		g_list_free(iter);
	}
	g_list_foreach(plugins_info, (GFunc) gel_plugin_info_free, NULL);
	g_list_free(plugins_info);

	// Try to load from NULL
	if ((info == NULL) && !(info = gel_plugin_info_new(NULL, name, NULL)))
	{
		g_set_error(error, app_quark(), GEL_PLUGIN_INFO_NOT_FOUND,
			N_("GelPluginInfo for '%s' not found"), name);
		return NULL;
	}

	GelPlugin *ret = gel_app_load_plugin(self, info, error);
	gel_plugin_info_free(info);

	return ret;
}

// --
// Unload plugins, _by_name and _by_pathname are provided via macros
// --
gboolean
gel_app_unload_plugin(GelApp *self, GelPlugin *plugin, GError **error)
{
	const GelPluginInfo *info = NULL;
	if (!GEL_IS_APP(self) || !plugin || !(info = gel_plugin_get_info(plugin)))
	{
		g_set_error(error, app_quark(), GEL_APP_ERROR_INVALID_ARGUMENTS, N_("Invalid arguments"));
		return FALSE;
	}

	// Call fini
	return gel_plugin_free(plugin, error);
}

// --
// Automatically purges all plugins which are neither initialized or locked
// Usefull to cleanup a GelApp after a query_plugins call
// --
void
gel_app_purge(GelApp *app)
{
	gel_warn(__FUNCTION__);
#if 0
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
#endif
}

// --
// Shared memory management
// --
gboolean gel_app_shared_set
(GelApp *self, gchar *name, gpointer data)
{
	g_return_val_if_fail(GEL_IS_APP(self), FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(g_hash_table_lookup(self->priv->shared, name) == NULL, FALSE);

	g_hash_table_insert(self->priv->shared, g_strdup(name), data);
	return TRUE;
}

gpointer gel_app_shared_get
(GelApp *self, gchar *name)
{
	g_return_val_if_fail(GEL_IS_APP(self), FALSE);
	g_return_val_if_fail(name != NULL, FALSE);

	gpointer ret = g_hash_table_lookup(self->priv->shared, name);
	g_return_val_if_fail(ret != NULL, FALSE);

	return ret;
}

void
gel_app_shared_free(GelApp *self, gchar *name)
{
	g_return_if_fail(GEL_IS_APP(self));
	g_return_if_fail(name != NULL);

	g_warn_if_fail(g_hash_table_lookup(self->priv->shared, name) != NULL);
	g_hash_table_remove(self->priv->shared, name);
}

// --
// Private funcs
// --
void
gel_app_priv_run_init(GelApp *self, GelPlugin *plugin)
{
	const GelPluginInfo *info = gel_plugin_get_info(plugin);

	// Add references
	if (info->depends)
	{
		gchar **deps = g_strsplit(info->depends, ",", 0);
		for (gint i = 0; deps[i] && deps[i][0]; i++)
		{
			GelPlugin *p = g_hash_table_lookup(self->priv->lookup, deps[i]);
			if (!p)
				g_warning(N_("Missing GelPlugin object '%s', cant add a reference. THIS IS A BUG."), deps[i]);
			else
				gel_plugin_add_reference(p, plugin);
		}
		g_strfreev(deps);
	}

	// Checks
	g_warn_if_fail(g_hash_table_lookup(self->priv->lookup, gel_plugin_get_info(plugin)->name) == NULL);
	g_warn_if_fail(g_queue_find(self->priv->stack, plugin) == NULL);

	// Insert into lookup table and stack
	g_hash_table_insert(self->priv->lookup, g_strdup(gel_plugin_get_info(plugin)->name), plugin);
	g_queue_push_tail(self->priv->stack, plugin);

	// Emit signal
	g_signal_emit(self, gel_app_signals[PLUGIN_INIT], 0, plugin);
}

void
gel_app_priv_run_fini(GelApp *self, GelPlugin *plugin)
{
	const GelPluginInfo *info = gel_plugin_get_info(plugin);

	g_signal_emit(self, gel_app_signals[PLUGIN_FINI], 0, plugin);

	g_warn_if_fail(g_queue_find(self->priv->stack, plugin) != NULL);
	g_warn_if_fail(g_hash_table_lookup(self->priv->lookup, gel_plugin_get_info(plugin)->name) != NULL);

	g_queue_remove(self->priv->stack, plugin);
	g_hash_table_remove(self->priv->lookup, gel_plugin_get_info(plugin)->name);

	// Remove references
	if (info->depends)
	{
		gchar **deps = g_strsplit(info->depends, ",", 0);
		for (gint i = 0; deps[i] && deps[i][0]; i++)
		{
			GelPlugin *p = g_hash_table_lookup(self->priv->lookup, deps[i]);
			if (!p)
				g_warning(N_("Missing GelPlugin object '%s', cant remove a reference. THIS IS A BUG."), deps[i]);
			else
				gel_plugin_remove_reference(p, plugin);
		}
		g_strfreev(deps);
	}
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

