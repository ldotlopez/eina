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

G_DEFINE_TYPE (GelPluginEngine, gel_plugin_engine, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_TYPE_APP, GelPluginEnginePrivate))

GEL_DEFINE_QUARK_FUNC(app)

static GList*
build_paths(void);

#define is_owned(self,plugin) \
	((gel_plugin_get_app(plugin) == self) && (g_hash_table_lookup(self->priv->lookup, gel_plugin_stringify(plugin)) != NULL))

struct _GelPluginEnginePrivate {
	GelPluginEngineDisposeFunc dispose_func;
	gpointer  dispose_data;

	GList      *paths;    // Paths to search plugins
	GList      *infos;    // Cached GelPluginInfo list

	GHashTable *lookup;   // Fast lookup table
	GQueue     *stack;
};

enum {
	PLUGIN_INIT,
	PLUGIN_FINI,

	LAST_SIGNAL
};
static guint gel_plugin_engine_signals[LAST_SIGNAL] = { 0 };

static void
gel_plugin_engine_dispose (GObject *object)
{
	GelPluginEngine *self = GEL_PLUGIN_ENGINE(object);

	if (self->priv == NULL)
	{
		G_OBJECT_CLASS (gel_plugin_engine_parent_class)->dispose (object);
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
		if (!gel_plugin_engine_unload_plugin(self, plugin, &error))
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

	gel_free_and_invalidate(self->priv->settings, NULL, g_hash_table_destroy);

	G_OBJECT_CLASS (gel_plugin_engine_parent_class)->dispose (object);
}

static void
gel_plugin_engine_finalize (GObject *object)
{
	G_OBJECT_CLASS (gel_plugin_engine_parent_class)->finalize (object);
}

static void
gel_plugin_engine_class_init (GelPluginEngineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	// g_type_class_add_private (klass, sizeof (GelPluginEnginePrivate));

	object_class->dispose = gel_plugin_engine_dispose;
	object_class->finalize = gel_plugin_engine_finalize;

    gel_plugin_engine_signals[PLUGIN_INIT] = g_signal_new ("plugin-init",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelPluginEngineClass, plugin_init),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    gel_plugin_engine_signals[PLUGIN_FINI] = g_signal_new ("plugin-fini",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelPluginEngineClass, plugin_fini),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
}

static void
gel_plugin_engine_init (GelPluginEngine *self)
{
	GelPluginEnginePriv *priv = self->priv = g_new0(GelPluginEnginePriv, 1);

	priv->dispose_func = priv->dispose_data = NULL;
	priv->paths  = NULL;
	priv->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
	priv->shared   = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	priv->lookup   = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	priv->stack    = g_queue_new();
	gel_plugin_engine_scan_plugins(self);
}

GelPluginEngine*
gel_plugin_engine_new (void)
{
	return g_object_new (GEL_TYPE_APP, NULL);
}

void
gel_plugin_engine_set_dispose_callback(GelPluginEngine *self, GelPluginEngineDisposeFunc callback, gpointer user_data)
{
	self->priv->dispose_func = callback;
	self->priv->dispose_data = user_data;
}

GList *
gel_plugin_engine_get_paths(GelPluginEngine *self)
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

GList*
gel_plugin_engine_get_plugins(GelPluginEngine *self)
{
	return g_hash_table_get_values(self->priv->lookup);
}

GelPlugin *gel_plugin_engine_get_plugin (GelPluginEngine *self, GelPluginInfo *info)
{
	GelPlugin *ret = NULL;

	GList *plugins = gel_plugin_engine_get_plugins(self);
	GList *iter = plugins;
	while (iter && !ret)
	{
		GelPlugin *plugin = GEL_PLUGIN(iter->data);
		if (gel_plugin_info_equal(gel_plugin_get_info(plugin), info))
			ret = plugin;
		iter = iter->next;
	}
	return ret;
}

GelPlugin *gel_plugin_engine_get_plugin_by_name(GelPluginEngine *self, gchar *name)
{
	return g_hash_table_lookup(self->priv->lookup, name);
}

GList *
gel_plugin_engine_query_plugins(GelPluginEngine *app)
{
	GList *ret = NULL;
	GList *iter = app->priv->infos;
	while (iter)
	{
		ret = g_list_prepend(ret, gel_plugin_info_dup((GelPluginInfo *) iter->data));
		iter = iter->next;
	}
	return g_list_reverse(ret);
}

void
gel_plugin_engine_scan_plugins(GelPluginEngine *app)
{
	if (app->priv->infos)
	{
		g_list_foreach(app->priv->infos, (GFunc) gel_plugin_info_free, NULL);
		g_list_free(app->priv->infos);
		app->priv->infos = NULL;
	}

	GList *paths = gel_plugin_engine_get_paths(app);
	GList *iter = paths;
	while (iter)
	{
		gchar *path = (gchar *) iter->data;

		GList *children = gel_dir_read(path, FALSE, NULL);
		GList *child = children;
		while (child)
		{
			gchar *p2 = (gchar *) child->data;
			gchar *ini = g_strconcat(p2, ".ini", NULL);
			gchar *infopath = g_build_filename(path, p2, ini, NULL);
			g_free(ini);

			GError *error = NULL;
			GelPluginInfo *info = gel_plugin_info_new(infopath, NULL, &error);
			if (!info)
			{
				gel_warn(N_("Cannot load plugin file '%s': %s"), infopath, error->message);
				g_error_free(error);
			}
			else
			{
				gel_warn(N_("Plugin file %s loaded"), infopath);
				app->priv->infos = g_list_prepend(app->priv->infos, info);
			}
			g_free(infopath);
			g_free(p2);

			child = child->next;
		}
		g_list_free(children);

		g_free(path);
		iter = iter->next;
	}
	g_list_free(paths);
}

// --
// Load and unload plugins
// --
GelPlugin *
gel_plugin_engine_load_plugin(GelPluginEngine *self, GelPluginInfo *info, GError **error)
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
			if (!gel_plugin_engine_load_plugin_by_name(self, deps[i], &err))
			{
 				g_set_error(error, app_quark(), GEL_PLUGIN_ENGINE_MISSING_PLUGIN_DEPS,
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
gel_plugin_engine_load_plugin_by_pathname(GelPluginEngine *self, gchar *pathname, GError **error)
{
	// Check if pathname is inside paths
	GList *paths = gel_plugin_engine_get_paths(self);

	gchar *basepath = g_path_get_dirname(pathname);
	gchar *dirname  = g_path_get_dirname(basepath);
	g_free(basepath);

	if (!g_list_find_custom(paths, dirname, (GCompareFunc) strcmp))
	{
		gel_warn("Invalid path");
		gel_list_deep_free(paths, g_free);
		g_free(dirname);
		return NULL;
	}
	g_free(dirname);
	gel_list_deep_free(paths, g_free);

	GelPluginInfo *info = gel_plugin_info_new(pathname, NULL, error);
	if (!info)
		return NULL;
	GelPlugin *ret = gel_plugin_engine_load_plugin(self, info, error);
	gel_plugin_info_free(info);
	return ret;
}

GelPlugin *
gel_plugin_engine_load_plugin_by_name(GelPluginEngine *self, gchar *name, GError **error)
{
	// gel_warn("%s (%p, %s,%p)", __FUNCTION__, self, name, error );

	// 1. Query plugins and try to match name against one of them
	GList *plugins_info = gel_plugin_engine_query_plugins(self);
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

	GelPlugin *ret = gel_plugin_engine_load_plugin(self, info, error);
	gel_plugin_info_free(info);

	return ret;
}

// --
// Unload plugins, _by_name and _by_pathname are provided via macros
// --
gboolean
gel_plugin_engine_unload_plugin(GelPluginEngine *self, GelPlugin *plugin, GError **error)
{
	const GelPluginInfo *info = NULL;
	if (!GEL_IS_APP(self) || !plugin || !(info = gel_plugin_get_info(plugin)))
	{
		g_set_error(error, app_quark(), GEL_PLUGIN_ENGINE_ERROR_INVALID_ARGUMENTS, N_("Invalid arguments"));
		return FALSE;
	}

	// Call fini
	return gel_plugin_free(plugin, error);
}

// --
// Automatically purges all plugins which are neither initialized or locked
// Usefull to cleanup a GelPluginEngine after a query_plugins call
// --
void
gel_plugin_engine_purge(GelPluginEngine *app)
{
	gel_warn(__FUNCTION__);
#if 0
	GList *plugins, *iter;
	plugins = iter = gel_plugin_engine_get_plugins(app);
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
// Private funcs
// --
void
gel_plugin_engine_priv_run_init(GelPluginEngine *self, GelPlugin *plugin)
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
	g_signal_emit(self, gel_plugin_engine_signals[PLUGIN_INIT], 0, plugin);
}

void
gel_plugin_engine_priv_run_fini(GelPluginEngine *self, GelPlugin *plugin)
{
	const GelPluginInfo *info = gel_plugin_get_info(plugin);

	g_signal_emit(self, gel_plugin_engine_signals[PLUGIN_FINI], 0, plugin);

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

