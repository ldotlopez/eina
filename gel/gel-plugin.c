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
	guint loads;
	guint inits;
};

static GQuark
gel_plugin_quark(void);

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
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_DYNAMIC_LOADING_NOT_SUPPORTED,
			N_("'%s' is not loadable"), pathname);
		return NULL;
	}

	if (!g_module_symbol(mod, symbol, &symbol_p))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_SYMBOL_NOT_FOUND,
			N_("Cannot find symbol '%s' in '%s'"), symbol, pathname);
		g_module_close(mod);
		return NULL;
	}

	if (GEL_PLUGIN(symbol_p)->serial != GEL_PLUGIN_SERIAL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_INVALID_SERIAL,
			N_("Invalid serial in %s (app:%d, plugin:%d)"), pathname, GEL_PLUGIN_SERIAL, GEL_PLUGIN(symbol_p)->serial);
		g_module_close(mod);
		return NULL;
	}

	self = GEL_PLUGIN(symbol_p);
	self->priv = g_new0(GelPluginPrivate, 1);
	self->priv->loads    = 1;
	self->priv->inits    = 0;
	self->priv->pathname = g_strdup(pathname);
	self->priv->symbol   = g_strdup(symbol);
	self->priv->module   = mod;
	self->priv->app      = app;
	return self;
}

gboolean
gel_plugin_free(GelPlugin *self, GError **error)
{
	if (gel_plugin_is_enabled(self))
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_STILL_ENABLED,
			N_("Cannot free GelPlugin '%s' it is still enabled"), gel_plugin_stringify(self));
		return FALSE;
	}

	if (self->priv->loads > 0)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_STILL_REFERENCED,
			N_("Cannot free GelPlugin '%s' it is still referenced"), gel_plugin_stringify(self));
		return FALSE;
	}	
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
	self->priv->loads++;
}

void
gel_plugin_unref(GelPlugin *self)
{
	if (self->priv->loads > 0)
		self->priv->loads--;
}

gboolean
gel_plugin_init(GelPlugin *self, GError **error)
{
	// Already initialized
	if (self->priv->inits)
	{
		self->priv->inits++;
		return TRUE;
	}

	// No init function, cannot init!
	if (self->init == NULL)
	{
		g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_HAS_NO_INIT_HOOK,
			N_("Cannot init GelPlugin %s, no init hook"), gel_plugin_stringify(self));
		return FALSE;
	}

	gboolean initialized = self->init(self, error);
	if (initialized)
		self->priv->inits = 1;
	else if (*error == NULL)
	{
		 g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NO_ERROR_AVAILABLE,
		 	N_("Init function for %s failed with no reason"), gel_plugin_stringify(self));
	}
	return initialized;
}

gboolean
gel_plugin_fini(GelPlugin *self, GError **error)
{
	if (self->priv->inits == 0)
	{
		g_warning(N_("Trying to fini GelPlugin %s which has inits == 0"), gel_plugin_stringify(self));
		return TRUE;
	}

	// Fake successful if there are more han 1 caller
	if (self->priv->inits > 1)
	{
		self->priv->inits--;
		return TRUE;
	}

	gboolean finalized = FALSE;

	// Call fini hook
	if (self->fini == NULL)
		finalized = TRUE;
	else
		finalized = self->fini(self, error);
	
	if (finalized)
		self->priv->inits--;
	else if (*error == NULL)
	{
		 g_set_error(error, gel_plugin_quark(), GEL_PLUGIN_NO_ERROR_AVAILABLE,
		 	N_("Fini function for %s failed with no reason"), gel_plugin_stringify(self));
	}

	return finalized;
}

guint
gel_plugin_get_loads(GelPlugin *self)
{
	return self->priv->loads;
}

guint
gel_plugin_get_inits(GelPlugin *self)
{
	return self->priv->inits;
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
gel_plugin_is_enabled(GelPlugin *self)
{
	return (self->priv->inits > 0);
}

static GQuark
gel_plugin_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("gel-plugin");
	return ret;

}
