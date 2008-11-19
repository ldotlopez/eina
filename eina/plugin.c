#define GEL_DOMAIN "Eina::IFace"

#include <string.h> // strcmp
#include <gmodule.h>
#include <gel/gel.h>
#include "base.h"
#include "iface.h"

struct _EinaPluginPrivate {
	gboolean    enabled;
	gchar      *pathname;
	GModule    *module;

	GelHub     *hub;
	LomoPlayer *lomo;
};

// --
// Constructor and destructor
// --
EinaPlugin*
eina_plugin_new(GelHub *hub, gchar *plugin_path)
{
	EinaPlugin *self= NULL;
	GModule    *mod;
	gpointer    symbol;

	if (!g_module_supported())
	{
		gel_error("Module loading is NOT supported on this platform");
		return NULL;
	}

	if ((mod = g_module_open(plugin_path, G_MODULE_BIND_LAZY)) == NULL)
	{
		gel_warn("'%s' is not loadable", plugin_path);
		return NULL;
	}

	if (!g_module_symbol(mod, "eina_plugin", &symbol))
	{
		gel_warn("Cannot find symbol '%s' in '%s'", "eina_plugin", plugin_path);
		g_module_close(mod);
		return NULL;
	}

	if (((EinaPlugin *) symbol)->serial != EINA_PLUGIN_SERIAL)
	{
		gel_warn("Invalid serial in %s", plugin_path);
		g_module_close(mod);
		return NULL;
	}

	self = EINA_PLUGIN(symbol);
	self->priv = g_new0(EinaPluginPrivate, 1);
	self->priv->pathname    = g_strdup(plugin_path);
	self->priv->module      = mod;
	self->priv->hub         = hub;
	self->priv->lomo        = gel_hub_shared_get(hub, "lomo");

	gel_debug("Module '%s' has been loaded", plugin_path);
	return self;
}

gboolean
eina_plugin_free(EinaPlugin *self)
{
	if (self->priv->enabled)
		return FALSE;
	g_free(self->priv->pathname);
	g_module_close(self->priv->module);
	g_free(self->priv);
	return TRUE;
}

// --
// Init and fini functions
// --
gboolean
eina_plugin_init(EinaPlugin *self)
{
	GError *err = NULL;

	if (self->init == NULL)
	{
		gel_error("Plugin %s cannot be initialized: %s",
			self->priv->pathname,
			"No init hook");
		return FALSE;
	}

	if (!self->init(self, &err))
	{
		gel_error("Plugin %s cannot be initialized: %s",
			self->priv->pathname,
			(err != NULL) ? err->message : "No error");
		if (err != NULL)
			g_error_free(err);
		return FALSE;
	}
	
	self->priv->enabled = TRUE;
	return TRUE;
}

gboolean
eina_plugin_fini(EinaPlugin *self)
{
	GError *err = NULL;

	if (!self->fini(self, &err))
	{
		gel_error("Plugin %s cannot be finalized: %s",
			self->priv->pathname,
			(err != NULL) ? err->message : "No error");
		return FALSE;
	}
	
	self->priv->enabled = FALSE;

	return TRUE;
}

// --
// Functions needed to access private elements
// --
const gchar *
eina_plugin_get_pathname(EinaPlugin *plugin)
{
	return (const gchar *) plugin->priv->pathname;
}

gboolean
eina_plugin_is_enabled(EinaPlugin *plugin)
{
	return plugin->priv->enabled;
}

GelHub *
eina_plugin_get_hub(EinaPlugin *plugin)
{
	return plugin->priv->hub;
}

LomoPlayer*
eina_plugin_get_lomo(EinaPlugin *plugin)
{
	return plugin->priv->lomo;
}

