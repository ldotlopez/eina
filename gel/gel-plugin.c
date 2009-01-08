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
	gboolean enabled;
	guint refs;
};

static GQuark
gel_plugin_quark(void);

GelPlugin*
gel_plugin_new(GelApp *app, gchar *pathname, gchar *symbol, GError **error)
{
	GelPlugin *self = NULL;
	GModule    *mod;
	gpointer    symbol_p;

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
	self->priv->refs     = 1;
	self->priv->pathname = g_strdup(pathname);
	self->priv->symbol   = g_strdup(symbol);
	self->priv->module   = mod;
	self->priv->app      = app;
	// self->priv->lomo        = gel_hub_shared_get(hub, "lomo");
	return self;

}

gboolean gel_plugin_init(GelPlugin *self, GError **error)
{
	if (self->priv->enabled)
		return TRUE;

	if (self->init == NULL)
		return FALSE;

	return (self->priv->enabled = self->init(self, error));
}

gboolean gel_plugin_fini(GelPlugin *self, GError **error)
{
	if (!self->priv->enabled)
		return TRUE;

	if (self->fini == NULL)
	{
		self->priv->enabled = FALSE;
		return TRUE;
	}

	return !(self->priv->enabled = !self->fini(self, error));
}

void
gel_plugin_ref(GelPlugin *self)
{
	self->priv->refs++;
}

void
gel_plugin_unref(GelPlugin *self)
{
	self->priv->refs--;
	if (self->priv->refs > 0)
		return;

	if (self->priv->enabled)
	{
		g_warning(N_("Plugin is still enabled, cannot be destroyed"));
		return;
	}

	GelPluginPrivate *priv = self->priv;
	g_free(self->priv->pathname);
	g_free(self->priv->symbol);
	gel_free_and_invalidate(self->priv->stringified, NULL, g_free);
	g_module_close(priv->module);
	g_free(priv);
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
	if (self->priv->stringified)
		self->priv->stringified = g_strdup_printf("%s:%s", self->priv->pathname ?  self->priv->pathname : "<BUILD-IN>" , self->priv->symbol);
	return self->priv->stringified;
}

gboolean
gel_plugin_is_enabled(GelPlugin *self)
{
	return self->priv->enabled;
}

static GQuark
gel_plugin_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("gel-plugin");
	return ret;
}
