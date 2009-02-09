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

#include <gmodule.h>
#include <glib/gi18n.h>
#include <gel/gel-misc.h>
#include <gel/gel-plugin.h>

struct _GelPluginPrivate {
	GelApp  *app;
	gchar   *pathname;
	gchar   *symbol;
	gchar   *stringified;
	GModule *module;
	guint    refs;
	gboolean enabled;
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
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_CANNOT_LOAD, N_("Not loadable"));
		return NULL;
	}

	if (!g_module_symbol(mod, symbol, &symbol_p))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_SYMBOL_NOT_FOUND, N_("Cannot find symbol"));
		g_module_close(mod);
		return NULL;
	}

	if (GEL_PLUGIN(symbol_p)->serial != GEL_PLUGIN_SERIAL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_INVALID_SERIAL,
			N_("Invalid serial (app:%d, plugin:%d)"), GEL_PLUGIN_SERIAL, GEL_PLUGIN(symbol_p)->serial);
		g_module_close(mod);
		return NULL;
	}

	self = GEL_PLUGIN(symbol_p);
	self->priv = g_new0(GelPluginPrivate, 1);
	self->priv->refs     = 1;
	self->priv->enabled  = FALSE;
	self->priv->pathname = g_strdup(pathname);
	self->priv->symbol   = g_strdup(symbol);
	self->priv->module   = mod;
	self->priv->app      = app;

	gel_app_emit_load(self->priv->app, self);

	return self;
}

gboolean
gel_plugin_free(GelPlugin *self, GError **error)
{
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_STILL_ENABLED, N_("Still enabled"));
		return FALSE;
	}

	if (gel_plugin_get_usage(self) > 0)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_STILL_REFERENCED, N_("Still referenced"));
		return FALSE;
	}

	gel_app_emit_unload(self->priv->app, self);
	gel_app_delete_plugin(self->priv->app, self);

	gel_free_and_invalidate(self->priv->pathname, NULL, g_free);
	gel_free_and_invalidate(self->priv->symbol, NULL, g_free);
	gel_free_and_invalidate(self->priv->stringified, NULL, g_free);
	
	GelPluginPrivate *priv = self->priv;
	g_module_close(self->priv->module);
	gel_free_and_invalidate(priv, NULL, g_free);

	return TRUE;
}

void
gel_plugin_ref(GelPlugin *self)
{
	self->priv->refs++;
}

void
gel_plugin_unref(GelPlugin *self)
{
	// Warn if refcount already 0
	if (self->priv->refs == 0)
	{
		g_warning(N_("GelPlugin %s refcount catch a negative refcount. Ignoring, but this is a bug in someone's code"), gel_plugin_stringify(self));
		return;
	}

	// Warn and abort if ref count will reach 0 and still enabled
	if ((self->priv->refs == 1) && gel_plugin_is_enabled(self))
	{
		g_warning(N_("GelPlugin %s catch drop last reference while plugin enabled. Ignoring, but this is a bug in someone's code"), gel_plugin_stringify(self));
		return;
	}

	self->priv->refs--;
	if (self->priv->refs == 0)
	{
		GError *error = NULL;
		if (gel_plugin_is_enabled(self) && !gel_plugin_fini(self, &error))
		{
			g_warning(N_("Cannot finalize plugin %s: %s"), gel_plugin_stringify(self), error->message);
			self->priv->refs++;
			return;
		}
		gel_plugin_free(self, NULL);
	}
}

guint
gel_plugin_get_usage(GelPlugin *self)
{
	return self->priv->refs;
}

gboolean
gel_plugin_is_enabled(GelPlugin *self)
{
	return self->priv->enabled;
}

gboolean
gel_plugin_matches(GelPlugin *self, gchar *pathname, gchar *symbol)
{
	if ((self->priv->pathname == NULL) && (pathname == NULL))
		return g_str_equal(self->priv->symbol, symbol);

	return ((self->priv->pathname != NULL) && (pathname != NULL) &&
		g_str_equal(self->priv->pathname, pathname) && g_str_equal(self->priv->symbol, symbol));
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
		self->priv->stringified = g_strdup_printf("%s:%s",
			(self->priv->pathname ?  self->priv->pathname : "<BUILD-IN>" ),
			self->priv->symbol);
	return self->priv->stringified;
}

gboolean
gel_plugin_init(GelPlugin *self, GError **error)
{
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_ALREADY_INITIALIZED,
			N_("Already initialized"));
		return FALSE;
	}

	if (gel_plugin_get_usage(self) == 0)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NOT_REFERENCED,
			N_("Is not referenced by anyone"));
		return FALSE;
	}

	if (self->init == NULL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_HAS_NO_INIT_HOOK,
			N_("No init hook"));
		return FALSE;
	}

	gboolean initialized = self->priv->enabled = self->init(self->priv->app, self, error);
	if (!initialized && (*error == NULL))
	{
		 g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NO_ERROR_AVAILABLE,
		 	N_("No reason"));
	}

	if (initialized)
		gel_app_emit_init(self->priv->app, self);

	return initialized;
}

gboolean
gel_plugin_fini(GelPlugin *self, GError **error)
{
	if (!gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NOT_INITIALIZED,
			N_("Not initialized"));
		return FALSE;
	}

	// Call fini hook
	gboolean finalized;
	if (self->fini == NULL)
		finalized = TRUE;
	else
		finalized = self->fini(self->priv->app, self, error);
	
	self->priv->enabled = !finalized;
	if (!finalized && (*error == NULL))
	{
		 g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NO_ERROR_AVAILABLE,
		 	N_("No reason"));
	}

	if (finalized)
		gel_app_emit_fini(self->priv->app, self);

	return finalized;
}

