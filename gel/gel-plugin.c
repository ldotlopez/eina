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
#include <gel/gel-plugin-info.h>

struct _GelPlugin {
	GelPluginInfo *info;
	GelApp        *app;
	GModule       *module;
	GList         *references;
	gchar         *stringified;
	gboolean (*init) (GelApp *app, struct _GelPlugin *plugin, GError **error); // Init function
	gboolean (*fini) (GelApp *app, struct _GelPlugin *plugin, GError **error); // Exit function
	gpointer data;
};

GEL_DEFINE_QUARK_FUNC(plugin);

GelPlugin*
gel_plugin_new(GelApp *app, GelPluginInfo *info, GError **error)
{
	if (!g_module_supported())
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_NOT_SUPPORTED,
			N_("Module loading is not supported on this platform"));
		return NULL;
	}

	
	gchar *mod_filename = NULL;
	if (info->dirname)
	{
		gchar *pname = g_path_get_basename(info->dirname);
		mod_filename = g_module_build_path(info->dirname, pname);
		g_free(pname);
	}

	GModule *mod;
	if ((mod = g_module_open(mod_filename, G_MODULE_BIND_LAZY)) == NULL)
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_MODULE_NOT_LOADABLE,
			N_("%s is not loadable"), info->pathname);
		g_free(mod_filename);
		return NULL;
	}
	gel_free_and_invalidate(mod_filename, NULL, g_free);

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

	plugin->info     = gel_plugin_info_dup(info);
	plugin->init     = init;
	plugin->fini     = fini;
	plugin->app      = app;
	plugin->module   = mod;
	plugin->references = NULL;

	if (!plugin->init(app, plugin, error))
	{
		g_module_close(plugin->module);
		g_free(plugin);
		return NULL;
	}

	gel_app_priv_run_init(app, plugin);
	return plugin;
}

gboolean
gel_plugin_free(GelPlugin *self, GError **error)
{
	g_warn_if_fail(self->references == NULL);
	g_warn_if_fail(gel_plugin_is_in_use(self) == FALSE);

	// Plugin in use (referenced) cannot be destroyed
	if (gel_plugin_is_in_use(self))
	{
		g_set_error(error, plugin_quark(), GEL_PLUGIN_ERROR_IN_USE,
			N_("Plugin %s is in use"), gel_plugin_stringify(self));
		return FALSE;
	}

	if (self->fini && !self->fini(self->app, self, error))
	{	
		if (error && !*error)
			g_warning(N_("fini hook failed without a reason"));
		return FALSE;
	}

	gel_app_priv_run_fini(self->app, self);

	gel_free_and_invalidate(self->stringified, NULL, g_free);
	gel_plugin_info_free(self->info);
	g_module_close(self->module);
	g_free(self);

	return TRUE;
}

const GelPluginInfo*
gel_plugin_get_info(GelPlugin *plugin)
{
	return (const GelPluginInfo *) plugin->info;
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

const GList*
gel_plugin_get_dependants(GelPlugin *plugin)
{
	return (const GList *) plugin->references;
}

guint
gel_plugin_get_usage(GelPlugin *self)
{
	return g_list_length(self->references);
}

gboolean
gel_plugin_is_in_use(GelPlugin *self)
{
	return self->references != NULL;
}

void
gel_plugin_add_reference(GelPlugin *self, GelPlugin *dependant)
{
	if (g_list_find(self->references, dependant))
	{
		gel_warn("[@] '%s' already referenced by '%s'", self->info->name, dependant->info->name);
		return;
	}

	self->references = g_list_prepend(self->references, dependant);
	gel_warn("[@] '%s' -> '%s'", dependant->info->name, self->info->name);
}

void
gel_plugin_remove_reference(GelPlugin *self, GelPlugin *dependant)
{
	GList *p = g_list_find(self->references, dependant);
	if (p == NULL)
	{
		gel_error(N_("Invalid reference from %s on %s"),
			gel_plugin_stringify(dependant),
			gel_plugin_stringify(self));
		return;
	}
	self->references = g_list_remove_link(self->references, p);
	g_list_free(p);

	 gel_warn("[@] '%s' w '%s'", dependant->info->name, self->info->name);
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
		self->stringified = g_strdup_printf("%s:%s",
			self->info->pathname ? self->info->pathname : "<NULL>",
			self->info->name
			);
	return self->stringified;
}

