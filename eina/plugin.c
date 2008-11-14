#define GEL_DOMAIN "Eina::IFace"

#include <string.h> // strcmp
#include <gmodule.h>
#include <gel/gel.h>
#include "base.h"
#include "iface.h"

struct _EinaPluginPrivate {
	gboolean    enabled;
	gchar      *plugin_name;
	gchar      *pathname;
	GModule    *module;

	EinaIFace  *iface;
	LomoPlayer *lomo;
};

// --
// Functions needed to access internal and deeper elements
// --

const gchar *
eina_plugin_get_pathname(EinaPlugin *plugin)
{
	return (const gchar *) plugin->priv->pathname;
}

gboolean
eina_plugin_get_enabled(EinaPlugin *plugin)
{
	return plugin->priv->enabled;
}

EinaIFace *
eina_plugin_get_iface(EinaPlugin *plugin)
{
	return plugin->priv->iface;
}

LomoPlayer*
eina_plugin_get_lomo(EinaPlugin *plugin)
{
	return plugin->priv->lomo;
}

// --
// Constructor and destructor
// --
EinaPlugin*
eina_plugin_new(EinaIFace *iface, gchar *plugin_name, gchar *plugin_path)
{
	EinaPlugin *self= NULL;
	GModule    *mod;
	gchar      *symbol_name;
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

	symbol_name = g_strconcat(plugin_name, "_plugin", NULL);

	if (!g_module_symbol(mod, symbol_name, &symbol))
	{
		gel_warn("Cannot find symbol '%s' in '%s'", symbol_name, plugin_path);
		g_free(symbol_name);
		g_module_close(mod);
		return NULL;
	}
	g_free(symbol_name);

	if (((EinaPlugin *) symbol)->serial != EINA_PLUGIN_SERIAL)
	{
		gel_warn("Invalid serial in %s", plugin_path);
		g_module_close(mod);
		return NULL;
	}

	self = (EinaPlugin *) symbol;
	self->priv = g_new0(EinaPluginPrivate, 1);
	self->priv->plugin_name = g_strdup(plugin_name);
	self->priv->pathname    = g_strdup(plugin_path);
	self->priv->module      = mod;
	self->priv->iface       = iface;
	self->priv->lomo        = gel_hub_shared_get(eina_iface_get_hub(iface), "lomo");

	gel_debug("Module '%s' has been loaded", plugin_path);
	return self;
}

gboolean
eina_plugin_free(EinaPlugin *self)
{
	if (self->priv->enabled)
		return FALSE;
	g_free(self->priv->plugin_name);
	g_free(self->priv->pathname);
	g_module_close(self->priv->module);
	g_free(self->priv);
	return TRUE;
}

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

