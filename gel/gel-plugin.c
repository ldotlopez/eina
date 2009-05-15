/*
 * gel/gel-plugin.c
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
#include <gmodule.h>
#include <glib/gi18n.h>
#include <gel/gel-misc.h>
#include <gel/gel-plugin.h>

struct _GelPluginPrivate {
	GelApp   *app;
	gchar    *pathname;
	gchar    *symbol;
	gchar    *stringified;
	GModule  *module;
	gboolean  enabled;
	GList    *dependants;
	guint     locks;
};

struct _GelPluginExtend {
	gpointer dummy;
};

static GQuark
gel_plugin_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("gel-plugin");
	return ret;

}

GelPlugin*
gel_plugin_new(GelApp *app, gchar *pathname, gchar *symbol, GError **error)
{
	GelPlugin *self = NULL;
	GModule   *mod;
	gpointer   symbol_p;

	if (!g_module_supported())
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_DYNAMIC_LOADING_NOT_SUPPORTED,
			N_("Module loading is NOT supported on this platform"));
		return NULL;
	}

	if ((mod = g_module_open(pathname, G_MODULE_BIND_LAZY)) == NULL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_CANNOT_LOAD, N_("%s is not loadable"), pathname);
		return NULL;
	}

	if (!g_module_symbol(mod, symbol, &symbol_p))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_SYMBOL_NOT_FOUND, N_("Cannot find symbol %s in %s"), symbol, pathname);
		g_module_close(mod);
		return NULL;
	}

	if (GEL_PLUGIN(symbol_p)->serial != GEL_PLUGIN_SERIAL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_INVALID_SERIAL,
			N_("Invalid serial in object %s (app:%d, plugin:%d)"), pathname, GEL_PLUGIN_SERIAL, GEL_PLUGIN(symbol_p)->serial);
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

#if 0
	gel_warn("== Plugin: %s", self->priv->pathname);
	gel_warn("Name: %s (%s), depends on: %s", self->name, self->version, self->depends);
	gel_warn("Author: %s (%s)", self->author, self->url);
	gel_warn("Desc: (%s) (%s) (%s)", self->short_desc, self->long_desc, self->icon);
	gel_warn("Init/Fini %p %p", self->init, self->fini);
	gel_warn(" ");
#endif

	gel_warn("New plugin %s created", gel_plugin_stringify(self));
	gel_app_add_plugin(app, self);

	return self;
}

gboolean
gel_plugin_free(GelPlugin *self, GError **error)
{
	// Plugin in use (referenced) cannot be destroyed
	if (gel_plugin_is_in_use(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_STILL_REFERENCED,
			N_("Plugin %s is in use"), gel_plugin_stringify(self));
		return FALSE;
	}

	// Plugin is not referenced, but it is enabled
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_STILL_ENABLED,
			N_("Plugin %s is still enabled"), gel_plugin_stringify(self));
		return FALSE;
	}

	// Check locks
	if (self->priv->locks != 0)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_ERROR_LOCKED,
			N_("Plugin %s is locked"), gel_plugin_stringify(self));
		return FALSE;
	}

	gel_warn("Plugin %s destroyed", gel_plugin_stringify(self));
	gel_app_remove_plugin(self->priv->app, self);

	gel_free_and_invalidate(self->priv->pathname, NULL, g_free);
	gel_free_and_invalidate(self->priv->symbol, NULL, g_free);
	gel_free_and_invalidate(self->priv->stringified, NULL, g_free);
	
	GelPluginPrivate *priv = self->priv;
	g_module_close(self->priv->module);
	gel_free_and_invalidate(priv, NULL, g_free);

	return TRUE;
}

gboolean
gel_plugin_is_locked(GelPlugin *plugin)
{
	return plugin->priv->locks > 0;
}

void
gel_plugin_add_lock(GelPlugin *plugin)
{
	plugin->priv->locks++;
}

void
gel_plugin_remove_lock(GelPlugin *plugin)
{
	if (plugin->priv->locks > 0)
		plugin->priv->locks--;
}

gboolean
gel_plugin_is_enabled(GelPlugin *self)
{
	return self->priv->enabled;
}

gboolean
gel_plugin_is_in_use(GelPlugin *self)
{
	return self->priv->dependants != NULL;
}

guint
gel_plugin_get_usage(GelPlugin *self)
{
	return g_list_length(self->priv->dependants);
}

void
gel_plugin_add_reference(GelPlugin *self, GelPlugin *dependant)
{
	GList *p = g_list_find(self->priv->dependants, dependant);
	if (p != NULL)
	{
		gel_error(N_("Invalid reference from %s on %s"),
			gel_plugin_stringify(dependant),
			gel_plugin_stringify(self));
		return;
	}
	self->priv->dependants = g_list_prepend(self->priv->dependants, dependant);

	gel_warn("Reference added on %s from %s", gel_plugin_stringify(self), gel_plugin_stringify(dependant));
	gel_warn("  current refs: %s", gel_plugin_util_join_list(", ", self->priv->dependants));
}

void
gel_plugin_remove_reference(GelPlugin *self, GelPlugin *dependant)
{
	GList *p = g_list_find(self->priv->dependants, dependant);
	if (p == NULL)
	{
		gel_error(N_("Invalid reference from %s on %s"),
			gel_plugin_stringify(dependant),
			gel_plugin_stringify(self));
		return;
	}
	self->priv->dependants = g_list_remove_link(self->priv->dependants, p);
	g_list_free(p);

	gel_warn("Reference removed on %s from %s", gel_plugin_stringify(self), gel_plugin_stringify(dependant));
	gel_warn("  current refs: %s", gel_plugin_util_join_list(", ", self->priv->dependants));
}

GelApp *
gel_plugin_get_app(GelPlugin *self)
{
	return self->priv->app;
}

const gchar *
gel_plugin_get_pathname(GelPlugin *self)
{
	return self->priv->pathname;
}

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

const gchar *
gel_plugin_stringify(GelPlugin *self)
{
	if (self->priv->stringified == NULL)
		self->priv->stringified = gel_plugin_util_stringify(self->priv->pathname, self->name);
	return self->priv->stringified;
}

gboolean
gel_plugin_init(GelPlugin *self, GError **error)
{
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_ALREADY_INITIALIZED,
			N_("Plugin %s already initialized"), gel_plugin_stringify(self));
		return FALSE;
	}

	if (self->init == NULL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_HAS_NO_INIT_HOOK,
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
			g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NO_ERROR_AVAILABLE,
		 		N_("Cannot init plugin %s, unknow reason"), gel_plugin_stringify(self));
		return FALSE;
	}

	// gel_warn("Plugin %s initialized", gel_plugin_stringify(self));
	gel_app_emit_init(self->priv->app, self);
	return TRUE;
}

gboolean
gel_plugin_fini(GelPlugin *self, GError **error)
{
	if (!gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NOT_INITIALIZED,
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
			g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NO_ERROR_AVAILABLE,
				N_("Cannot finalize plugin %s: unknow reason"), gel_plugin_stringify(self));
		return FALSE;
	}

	// gel_warn("Plugin %s finalized", gel_plugin_stringify(self));
	gel_app_emit_fini(self->priv->app, self);
	return TRUE;
}

gchar*
gel_plugin_stringify_dependants(GelPlugin *plugin)
{
	GList *strs = NULL;
	GList *iter = plugin->priv->dependants;
	while (iter)
	{
		strs = g_list_prepend(strs, (gpointer) gel_plugin_stringify(GEL_PLUGIN(iter->data)));
		iter = iter->next;
	}
	gchar *ret = gel_list_join(", ", strs);
	g_list_free(strs);

	return ret;
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


