/*
 * gel/gel-plugin.c
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
#include <gmodule.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <gel/gel-misc.h>
#include <gel/gel-plugin.h>

#if 0
struct _GelPluginPrivate {
	GelApp   *app;
	GModule  *module;
	gboolean  enabled;
	GList    *rdepends;
	gchar    *stringified;
#if 0
	gchar    *pathname;
	gchar    *symbol;
 	guint     locks;
#endif
};
#endif
struct _GelPlugin {
	GelPluginInfo *info;
	GelApp        *app;
	GModule       *module;
	GList         *rdepends;
	gchar         *stringified;
	gboolean (*init) (GelApp *app, struct _GelPlugin *plugin, GError **error); // Init function
	gboolean (*fini) (GelApp *app, struct _GelPlugin *plugin, GError **error); // Exit function
	gpointer data;
};

struct _GelPluginExtend {
	gpointer dummy;
};


GEL_DEFINE_QUARK_FUNC(plugin);

GelPluginInfo *
gel_plugin_info_new(const gchar *filename, const gchar *name ,GError **error)
{
	if ((filename == NULL) && (name == NULL))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_INFO_NULL_FILENAME, N_("A name must be provided if filename is NULL"));
		g_return_val_if_fail((filename != NULL) && (name != NULL), NULL);
	}

	GelPluginInfo *pinfo = g_new0(GelPluginInfo, 1);

	// Load as internal
	if (filename == NULL)
	{
		GModule *m = g_module_open(NULL, G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL);
		if (!m)
		{
			g_set_error(error, plugin_quark(), GEL_PLUGIN_INFO_CANNOT_OPEN_MODULE, N_("Unable to introspect NULL"));
			return NULL;
		}

		gchar *symbol = g_strconcat(name, "_plugin_info", NULL);
		GelPluginInfo *symbol_p = NULL;
		if (!g_module_symbol(m, symbol, (gpointer *) &symbol_p))
		{
			g_set_error(error, plugin_quark(), GEL_PLUGIN_INFO_SYMBOL_NOT_FOUND, N_("Symbol '%s' not found"), symbol);
			g_free(symbol);
			g_module_close(m);
			return NULL;
		}
		gel_plugin_info_copy(symbol_p, pinfo);
		g_module_close(m);
	}

	// Load as external
	else
	{
		GKeyFile *kf = g_key_file_new();
		if (!g_key_file_load_from_file(kf, filename, 0, error))
		{
			g_key_file_free(kf);
			return NULL;
		}

		if (!g_key_file_has_group(kf, "Eina Plugin"))
		{	
			g_set_error(error, plugin_quark(), GEL_PLUGIN_INFO_HAS_NO_GROUP, N_("Info file has no group named 'Eina Plugin'"));
			g_key_file_free(kf);
			return NULL;
		}

		gchar *required_keys[] = {"name", "version", "url", "author", "depends"};
		for (gint i = 0; i < G_N_ELEMENTS(required_keys); i++)
		{
			if (!g_key_file_has_key(kf, "Eina Plugin", required_keys[i], NULL))
			{
				g_set_error(error, plugin_quark(), GEL_PLUGIN_INFO_MISSING_KEY, N_("Missing required key '%s'"), required_keys[i]);
				g_key_file_free(kf);
				return NULL;
			}
		}

		pinfo->pathname = g_strdup(filename);
		pinfo->dirname  = g_path_get_dirname(filename);
		pinfo->name     = g_key_file_get_string(kf, "Eina Plugin", "name",    NULL);

		pinfo->version  = g_key_file_get_string(kf, "Eina Plugin", "version", NULL);
		pinfo->depends  = g_key_file_get_string(kf, "Eina Plugin", "depends", NULL);
		pinfo->author   = g_key_file_get_string(kf, "Eina Plugin", "author",  NULL);
		pinfo->url      = g_key_file_get_string(kf, "Eina Plugin", "url",     NULL);

		pinfo->short_desc = g_key_file_get_string(kf, "Eina Plugin", "short-desc", NULL);
		pinfo->long_desc  = g_key_file_get_string(kf, "Eina Plugin", "long-desc",  NULL);
		pinfo->icon_pathname = g_key_file_get_string(kf, "Eina Plugin", "icon-pathname", NULL);
	}

	return pinfo;
}

void
gel_plugin_info_free(GelPluginInfo *pinfo)
{
	gel_free_and_invalidate(pinfo->dirname,  NULL, g_free);
	gel_free_and_invalidate(pinfo->pathname, NULL, g_free);

	gel_free_and_invalidate(pinfo->version, NULL, g_free);
	gel_free_and_invalidate(pinfo->depends, NULL, g_free);
	gel_free_and_invalidate(pinfo->author,  NULL, g_free);
	gel_free_and_invalidate(pinfo->url ,    NULL, g_free);

	gel_free_and_invalidate(pinfo->short_desc, NULL, g_free);
	gel_free_and_invalidate(pinfo->long_desc,  NULL, g_free);
	gel_free_and_invalidate(pinfo->icon_pathname, NULL, g_free);
	gel_free_and_invalidate(pinfo->name,     NULL, g_free);
}

gboolean
gel_plugin_info_cmp(GelPluginInfo *a, GelPluginInfo *b)
{
	if (!a->pathname && b->pathname)
		return 1;
	if (a->pathname && !b->pathname)
		return -1;
	return strcmp(a->name, b->name);
}

void
gel_plugin_info_copy(GelPluginInfo *src, GelPluginInfo *dst)
{
	dst->name = src->name ? g_strdup(src->name) : NULL;
	dst->dirname  = src->dirname  ? g_strdup(src->dirname) : NULL;
	dst->pathname = src->pathname ? g_strdup(src->pathname) : NULL;

	dst->version = src->version ? g_strdup(src->version) : NULL;
	dst->depends = src->depends ? g_strdup(src->depends) : NULL;
	dst->author  = src->author  ? g_strdup(src->author)  : NULL;
	dst->url     = src->url     ? g_strdup(src->url)     : NULL;

	dst->short_desc = src->short_desc ? g_strdup(src->short_desc) : NULL;
	dst->long_desc  = src->long_desc  ? g_strdup(src->long_desc)  : NULL;
	dst->icon_pathname = src->icon_pathname ? g_strdup(src->icon_pathname) : NULL;
}

GelPlugin*
gel_plugin_new(GelApp *app, GelPluginInfo *info, GError **error)
{
	if (!g_module_supported())
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_NOT_SUPPORTED,
			N_("Module loading is not supported on this platform"));
		return NULL;
	}

	GModule *mod;
	if ((mod = g_module_open(info->pathname, G_MODULE_BIND_LAZY)) == NULL)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_MODULE_NOT_LOADABLE,
			N_("%s is not loadable"), info->pathname);
		return NULL;
	}

	gchar *init_func = g_strconcat(info->name, "_plugin_init", NULL);
	gchar *fini_func = g_strconcat(info->name, "_plugin_fini", NULL);
	gpointer init = NULL, fini = NULL;
	g_module_symbol(mod, init_func, &init);
	g_module_symbol(mod, fini_func, &fini);
	g_free(init_func);
	g_free(fini_func);

	if (!init)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_NO_INIT_HOOK, 
			N_("Plugin '%s' has no init hook"), info->name);
		g_module_close(mod);
		return NULL;
	}

	GelPlugin *plugin = g_new0(GelPlugin, 1);

	plugin->info = g_new0(GelPluginInfo, 1);
	gel_plugin_info_copy(info, plugin->info);

	plugin->init = init;
	plugin->fini = fini;
	plugin->app = app;
	plugin->module = mod;
	plugin->rdepends = NULL;

	if (!plugin->init(app, plugin, error))
	{
		g_module_close(plugin->module);
		g_free(plugin);
		return NULL;
	}

	gel_app_priv_run_init(app, plugin);
	return plugin;

#if 0
	GelPlugin *self = NULL;
	GModule   *mod;
	gpointer   symbol_p;

	if (!g_module_supported())
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_NOT_SUPPORTED,
			N_("Module loading is not supported on this platform"));
		return NULL;
	}

	if ((mod = g_module_open(pathname, G_MODULE_BIND_LAZY)) == NULL)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_MODULE_NOT_LOADABLE,
			N_("%s is not loadable"), pathname);
		return NULL;
	}

	if (!g_module_symbol(mod, symbol, &symbol_p))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_SYMBOL_NOT_FOUND,
			N_("Cannot find symbol %s in %s"), symbol, pathname);
		g_module_close(mod);
		return NULL;
	}

	if (GEL_PLUGIN(symbol_p)->serial != GEL_PLUGIN_SERIAL)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_INVALID_SERIAL,
			N_("Invalid serial in object %s:%s (app:%d, plugin:%d)"),
			pathname ? pathname : "<NULL>", symbol, GEL_PLUGIN_SERIAL, GEL_PLUGIN(symbol_p)->serial);
		g_module_close(mod);
		return NULL;
	}

	self = GEL_PLUGIN(symbol_p);
	self->priv = g_new0(GelPluginPrivate, 1);
	self->priv->enabled  = FALSE;
	self->priv->pathname = g_strdup(pathname);
	self->priv->symbol   = g_strdup(symbol);
	self->priv->module   = mod;
	self->priv->app      = app;

	if ((self->depends == NULL) || g_str_equal(self->depends, ""))
		gel_warn(N_("%s plugin has no deps, are you sure? if you are sure use GEL_PLUGIN_NO_DEPS"), gel_plugin_stringify(self));
	if (self->depends && g_str_equal(self->depends, GEL_PLUGIN_NO_DEPS))
		self->depends = NULL;
#if 0
	gel_warn("== Plugin: %s", self->priv->pathname);
	gel_warn("Name: %s (%s), depends on: %s", self->name, self->version, self->depends);
	gel_warn("Author: %s (%s)", self->author, self->url);
	gel_warn("Desc: (%s) (%s) (%s)", self->short_desc, self->long_desc, self->icon);
	gel_warn("Init/Fini %p %p", self->init, self->fini);
	gel_warn(" ");
#endif

	gel_debug(N_("Plugin %s created"), gel_plugin_stringify(self));
	gel_app_add_plugin(app, self);

	return self;
#endif
}

gboolean
gel_plugin_free(GelPlugin *self, GError **error)
{
	g_warn_if_fail(self->rdepends == NULL);
	g_warn_if_fail(gel_plugin_is_in_use(self) == FALSE);

	// Plugin in use (referenced) cannot be destroyed
	if (gel_plugin_is_in_use(self))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_IN_USE,
			N_("Plugin %s is in use"), gel_plugin_stringify(self));
		return FALSE;
	}

	if (self->fini && !self->fini(self->app, self, error))
		return FALSE;

	gel_app_priv_run_fini(self->app, self);

	gel_plugin_info_free(self->info);
	g_module_close(self->module);
	gel_free_and_invalidate(self->stringified, NULL, g_free);

	return TRUE;

#if 0
	// Plugin in use (referenced) cannot be destroyed
	if (gel_plugin_is_in_use(self))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_IN_USE,
			N_("Plugin %s is in use"), gel_plugin_stringify(self));
		return FALSE;
	}

	// Plugin is not referenced, but it is enabled
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_IS_ENABLED,
			N_("Plugin %s is still enabled"), gel_plugin_stringify(self));
		return FALSE;
	}

	// Check locks
	if (self->priv->locks != 0)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_LOCKED,
			N_("Plugin %s is locked"), gel_plugin_stringify(self));
		return FALSE;
	}

	gel_debug(N_("Plugin %s destroyed"), gel_plugin_stringify(self));
	gel_app_remove_plugin(self->priv->app, self);

	gel_free_and_invalidate(self->priv->pathname, NULL, g_free);
	gel_free_and_invalidate(self->priv->symbol, NULL, g_free);
	gel_free_and_invalidate(self->priv->stringified, NULL, g_free);
	
	GelPluginPrivate *priv = self->priv;
	g_module_close(self->priv->module);
	gel_free_and_invalidate(priv, NULL, g_free);

	return TRUE;
#endif
}

GelPluginInfo*
gel_plugin_get_info(GelPlugin *plugin)
{
	return plugin->info;
}

gpointer
gel_plugin_get_data(GelPlugin *plugin)
{
	return plugin->data;
}

void
gel_plugin_set_data(GelPlugin *plugin, gpointer data)
{
	plugin->data = data;
}

gboolean
gel_plugin_is_in_use(GelPlugin *self)
{
	return self->rdepends != NULL;
}

guint
gel_plugin_get_usage(GelPlugin *self)
{
	return g_list_length(self->rdepends);
}

void
gel_plugin_add_reference(GelPlugin *self, GelPlugin *dependant)
{
	if (g_list_find(self->rdepends, dependant))
	{
		gel_warn("[@] '%s' already referenced by '%s'", self->info->name, dependant->info->name);
		return;
	}

	self->rdepends = g_list_prepend(self->rdepends, dependant);
	gel_warn("[@] '%s' -> '%s'", dependant->info->name, self->info->name);
#if 0
	if (p != NULL)
	{
		gel_error(N_("Invalid reference from %s on %s"),
			gel_plugin_stringify(dependant),
			gel_plugin_stringify(self));
		return;
	}
	self->priv->rdepends = g_list_prepend(self->priv->rdepends, dependant);
#endif
	/*
	gchar *refs = gel_plugin_util_join_list(", ", self->priv->rdepends);
	gel_warn("[^] %s (%s)", gel_plugin_stringify(self), gel_str_or_text(refs, N_("none")));
	gel_free_and_invalidate(refs, NULL, g_free);
	*/
}

void
gel_plugin_remove_reference(GelPlugin *self, GelPlugin *dependant)
{
	GList *p = g_list_find(self->rdepends, dependant);
	if (p == NULL)
	{
		gel_error(N_("Invalid reference from %s on %s"),
			gel_plugin_stringify(dependant),
			gel_plugin_stringify(self));
		return;
	}
	self->rdepends = g_list_remove_link(self->rdepends, p);
	g_list_free(p);

	/*
	gchar *refs = gel_plugin_util_join_list(", ", self->priv->rdepends);
	gel_warn("[v] %s (%s)", gel_plugin_stringify(self), gel_str_or_text(refs, N_("none")));
	g_free(refs);
	*/
}

GelApp *
gel_plugin_get_app(GelPlugin *self)
{
	return self->app;
}

const gchar *
gel_plugin_get_pathname(GelPlugin *self)
{
	return self->info->pathname;
}

#if 0
gchar *
gel_plugin_build_resource_path(GelPlugin *plugin, gchar *resource_path)
{
	gchar *ret = NULL;
	const gchar *pathname = gel_plugin_get_pathname(plugin);

	if (pathname != NULL)
	{
		gchar *dirname = g_path_get_dirname(pathname);
		ret = g_build_filename(dirname, resource_path, NULL);
		g_free(dirname);
	}
	else
		ret = g_build_filename(gel_get_package_name(), resource_path, NULL);

	return ret;
}
#endif

GList *
gel_plugin_get_resource_list(GelPlugin *plugin, GelResourceType type, gchar *resource)
{
	// Print warn if usage is incorrect
	if (plugin == NULL)
	{
		g_printf(N_("You call %s with plugin == NULL, its OK but:\n"
			"- %s is meant for use with plugin def\n"
			"- For general, non-plugin dependand version of %s use the wrapper gel_resource_locate\n"
			"- If none of previous points fits your needs, pass a plugin variable to this function\n"
			"Set breakpoint on '%s if plugin == 0' to find the caller for this invocation\n"),
			__FUNCTION__, __FUNCTION__, __FUNCTION__, __FUNCTION__
			);
	}
	if (plugin == (GelPlugin*) 0x1)
		plugin = NULL;

	// Search precedence
	// 1. Enviroment variables
	// If enviroment variable is NULL:
	//   2.1. User's dir based on GelPluginResourceType
	//   2.2. System or plugin dir based on GelPluginResourceType
	if (!plugin || !gel_plugin_get_pathname(plugin))
	{
		const gchar *searchpath = gel_resource_type_get_env(type);
		if (searchpath)
		{
			gchar **searchpaths = g_strsplit(searchpath, G_SEARCHPATH_SEPARATOR_S, -1);
		
			GList *ret = NULL;
			gint i = 0;
			while (searchpaths[i] && searchpaths[i][0])
			{
				ret = g_list_append(ret, g_strconcat(searchpaths[i], G_DIR_SEPARATOR_S, resource, NULL));
				i++;
			}
			g_strfreev(searchpaths);
			return ret;
		}
	}

	GList *ret = NULL;
	gchar *userdir = gel_resource_type_get_user_dir(type);
	if (userdir)
	{
		ret = g_list_prepend(ret, g_build_filename(userdir, resource, NULL));
		g_free(userdir);
	}

	if ((plugin == NULL) || (gel_plugin_get_pathname(plugin) == NULL))
	{
		gchar *systemdir = gel_resource_type_get_system_dir(type);
		if (systemdir)
		{
			ret = g_list_prepend(ret, g_build_filename(systemdir, resource, NULL));
			g_free(systemdir);
		}
	}
	else
	{
		gchar *plugindir = g_path_get_dirname(gel_plugin_get_pathname(plugin));
		const gchar *map_table[GEL_N_RESOURCES] = { "ui", "pixmaps", "lib", "." };
		ret = g_list_prepend(ret, g_build_filename(plugindir, resource, NULL));
		ret = g_list_prepend(ret, g_build_filename(plugindir, map_table[type], resource, NULL));
		g_free(plugindir);
	}

	return g_list_reverse(ret);
}

gchar *
gel_plugin_get_resource(GelPlugin *plugin, GelResourceType type, gchar *resource)
{
	GList *candidates = gel_plugin_get_resource_list(plugin, type, resource);
	GList *iter = candidates;
	gchar *ret = NULL;
	while (iter)
	{
		if (g_file_test((gchar *) iter->data, G_FILE_TEST_IS_REGULAR))
		{
			ret = g_strdup((gchar *) iter->data);
			break;
		}
		iter = iter->next;
	}
	gel_list_deep_free(candidates, g_free);
	return ret;
}

const gchar *
gel_plugin_stringify(GelPlugin *self)
{
	if (self->stringified == NULL)
		self->stringified = gel_plugin_util_stringify(self->info->pathname, self->info->name);
	return self->stringified;
}

#if 0
gboolean
gel_plugin_init(GelPlugin *self, GError **error)
{
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_ALREADY_INITIALIZED,
			N_("Plugin %s already initialized"), gel_plugin_stringify(self));
		return FALSE;
	}

	if (self->init == NULL)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_NO_INIT_HOOK,
			N_("Plugin %s has not init hook"), gel_plugin_stringify(self));
		return FALSE;
	}

	if (!(self->priv->enabled = self->init(self->priv->app, self, error)))
	{
		if ((error != NULL) && (*error != NULL))
			gel_error(N_("Cannot init plugin %s: %s"),
				gel_plugin_stringify(self),
				(*error)->message);
		else
			g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_UNKNOW,
		 		N_("Cannot init plugin %s, unknow reason"), gel_plugin_stringify(self));
		return FALSE;
	}

	gel_debug(N_("Plugin %s initialized"), gel_plugin_stringify(self));
	gel_app_emit_init(self->priv->app, self);
	return TRUE;
}

gboolean
gel_plugin_fini(GelPlugin *self, GError **error)
{
	if (!gel_plugin_is_enabled(self))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_NOT_INITIALIZED,
			N_("Plugin %s is not initialized"), gel_plugin_stringify(self));
		return FALSE;
	}

	// Call fini hook
	gboolean finalized;
	if (self->fini == NULL)
	{
		self->priv->enabled = FALSE;
		finalized = TRUE;
	}
	else
	{
		finalized = self->fini(self->priv->app, self, error);
		self->priv->enabled = !finalized;
	}
	
	if (!finalized)
	{
		if ((error != NULL) && (*error != NULL))
			gel_error(N_("Cannot finalize plugin %s: %s"),
				gel_plugin_stringify(self),
				(*error)->message);
		else
			g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_UNKNOW,
				N_("Cannot finalize plugin %s: unknow reason"), gel_plugin_stringify(self));
		return FALSE;
	}

	gel_debug(N_("Plugin %s finalized"), gel_plugin_stringify(self));
	gel_app_emit_fini(self->priv->app, self);
	return TRUE;
}
#endif

const GList*
gel_plugin_get_dependants(GelPlugin *plugin)
{
	return (const GList *) plugin->rdepends;
}

gchar*
gel_plugin_stringify_dependants(GelPlugin *plugin)
{
	GList *strs = NULL;
	GList *iter = plugin->rdepends;
	if (iter == NULL)
		return NULL;

	while (iter)
	{
		strs = g_list_prepend(strs, (gpointer) gel_plugin_stringify(GEL_PLUGIN(iter->data)));
		iter = iter->next;
	}
	gchar *ret = gel_list_join(", ", strs);
	g_list_free(strs);

	return ret;
}

gint
gel_plugin_compare_by_name (GelPlugin *a, GelPlugin *b)
{
	const gchar *pa, *pb;
	pa = gel_plugin_get_pathname(a);
	pb = gel_plugin_get_pathname(b);

	if (pa && !pb)
		return 1;
	if (!pa && pb)
		return -1;
	return strcmp(a->info->name, b->info->name);
}

gint
gel_plugin_compare_by_usage(GelPlugin *a, GelPlugin *b)
{
	return gel_plugin_get_usage(b) - gel_plugin_get_usage(a);
}

gchar*
gel_plugin_util_symbol_from_pathname(gchar *pathname)
{
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *plugin_name = g_path_get_basename(dirname);
	g_free(dirname);
	gchar *symbol_name = g_strconcat(plugin_name, "_plugin", NULL);
	g_free(plugin_name);
	return symbol_name;
}

gchar*
gel_plugin_util_stringify_for_pathname(gchar *pathname)
{
	g_return_val_if_fail(pathname != NULL, NULL);
	gchar *dirname = g_path_get_dirname(pathname);
	gchar *name = g_path_get_basename(dirname);
	g_free(dirname);

	gchar *ret = g_build_filename(pathname, name, NULL);
	g_free(name);

	return ret;
}

gchar*
gel_plugin_util_join_list(gchar *separator, GList *plugin_list)
{
	GList *pstrs = NULL;
	while (plugin_list)
	{
		pstrs = g_list_append(pstrs, g_strdup(gel_plugin_stringify((GelPlugin *) plugin_list->data)));
		plugin_list = plugin_list->next;
	}
	gchar *ret = pstrs ? gel_list_join(separator, pstrs) : g_strdup("(null)");
	gel_list_deep_free(pstrs, g_free);
	return ret;
}

