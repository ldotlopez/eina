#define GEL_DOMAIN "Eina::Plugin"

#include <string.h> // strcmp
#include <gmodule.h>
#include <gel/gel.h>
#include "base.h"
#include "plugin.h"
#include "dock.h"
#include "preferences.h"

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
eina_plugin_new(GelHub *hub, gchar *plugin_path, gchar *symbol)
{
	EinaPlugin *self = NULL;
	GModule    *mod;
	gpointer    symbol_p;

	if (!g_module_supported())
	{
		gel_error("Module loading is NOT supported on this platform");
		return NULL;
	}

	if ((mod = g_module_open(plugin_path, G_MODULE_BIND_LAZY)) == NULL)
	{
		gel_info("'%s' is not loadable", plugin_path);
		return NULL;
	}

	if (!g_module_symbol(mod, symbol, &symbol_p))
	{
		gel_info("Cannot find symbol '%s' in '%s'", symbol, plugin_path);
		g_module_close(mod);
		return NULL;
	}

	if (EINA_PLUGIN(symbol_p)->serial != EINA_PLUGIN_SERIAL)
	{
		gel_warn("Invalid serial in %s (app:%d, plugin:%d)", plugin_path, EINA_PLUGIN_SERIAL, EINA_PLUGIN(symbol_p)->serial);
		g_module_close(mod);
		return NULL;
	}

	self = EINA_PLUGIN(symbol_p);
	self->priv = g_new0(EinaPluginPrivate, 1);
	self->priv->pathname    = g_strdup(plugin_path);
	self->priv->module      = mod;
	self->priv->hub         = hub;
	self->priv->lomo        = gel_hub_shared_get(hub, "lomo");

	// gel_debug("Module '%s' has been loaded", plugin_path);
	// gel_warn("Created (%p)%s", self, eina_plugin_get_pathname(self));
	return self;
}

gboolean
eina_plugin_free(EinaPlugin *self)
{
	EinaPluginPrivate *priv = self->priv;

	if (self->priv->enabled)
	{
		gel_error("Trying to close a plugin before fini it");
		return FALSE;
	}

	g_free(self->priv->pathname);
	if (!g_module_close(self->priv->module))
		return FALSE;

	g_free(priv);
	// Dont free self, self is inside shared object!

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
inline const gchar *
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

// --
// Utilities for plugins
// --
gchar *
eina_plugin_build_resource_path(EinaPlugin *plugin, gchar *resource)
{
	gchar *dirname, *ret;
	dirname = g_path_get_dirname(plugin->priv->pathname);

	ret = g_build_filename(dirname, resource, NULL);
	g_free(dirname);

	return ret;
}

gchar *
eina_plugin_build_userdir_path (EinaPlugin *self, gchar *path)
{
	gchar *dirname = g_path_get_dirname(self->priv->pathname);
	gchar *bname   = g_path_get_basename(dirname);
	gchar *ret     = g_build_filename(g_get_home_dir(), ".eina", bname, path, NULL);
	g_free(bname);
	g_free(dirname);
	
	return ret;
	// return g_build_filename(g_get_home_dir(), ".eina", plugin->priv->plugin_name, path, NULL);
}

// --
// Player management
// --
EinaPlayer *
eina_plugin_get_player(EinaPlugin *self)
{
	GelHub     *hub;
	EinaPlayer *ret;

	if ((hub = eina_plugin_get_hub(self)) == NULL)
		return NULL;
	if ((ret = gel_hub_shared_get(hub, "player")) == NULL)
		return NULL;
	return ret;
}

GtkWindow *
eina_plugin_get_main_window(EinaPlugin *self)
{
	EinaPlayer *player;

	if ((player = eina_plugin_get_player(self)) == NULL)
		return NULL;
	return GTK_WINDOW(W(player, "main-window"));
}

// --
// Dock management
// --
gboolean eina_plugin_add_dock_widget(EinaPlugin *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	return eina_dock_add_widget(GEL_HUB_GET_DOCK(self->priv->hub), id, label, dock_widget);
}

gboolean eina_plugin_dock_remove_item(EinaPlugin *self, gchar *id)
{
	return eina_dock_remove_widget(GEL_HUB_GET_DOCK(self->priv->hub), id);
}

gboolean
eina_plugin_dock_switch(EinaPlugin *self, gchar *id)
{
	return eina_dock_switch_widget(GEL_HUB_GET_DOCK(self->priv->hub), id);
}

// --
// Settings management
// --
static EinaPreferencesDialog *
eina_plugin_get_preferences(EinaPlugin *self)
{
	if (self->priv->hub == NULL)
		return NULL;
	return GEL_HUB_GET_PREFERENCES(self->priv->hub);
}

gboolean eina_plugin_add_configuration_widget
(EinaPlugin *plugin, GtkImage *icon, GtkLabel *label, GtkWidget *widget)
{
	EinaPreferencesDialog *prefs;
	prefs = eina_plugin_get_preferences(plugin);
	if (prefs == NULL)
		return FALSE;
	eina_preferences_dialog_add_tab(prefs, icon, label, widget);
	return TRUE;
}

gboolean eina_plugin_remove_configuration_widget
(EinaPlugin *plugin,  GtkWidget *widget)
{
	EinaPreferencesDialog *prefs;
	prefs = eina_plugin_get_preferences(plugin);
	if (prefs == NULL)
		return FALSE;
	eina_preferences_dialog_remove_tab(prefs, widget);
	return TRUE;
}


// --
// Cover management
// --
EinaCover*
eina_plugin_get_player_cover(EinaPlugin *self)
{
	EinaPlayer *player;
	
	player = GEL_HUB_GET_PLAYER(self->priv->hub);
	if (player == NULL)
		return NULL;

	// return eina_player_get_cover(player);
	return NULL;
}

void
eina_plugin_cover_add_backend(EinaPlugin *plugin, gchar *id,
    EinaCoverBackendFunc search, EinaCoverBackendCancelFunc cancel)
{
	EinaCover *cover = eina_plugin_get_player_cover(plugin);
	if (cover == NULL)
	{
		gel_warn("Unable to access cover");
		return;
	}
	eina_cover_add_backend(cover, id, search, cancel, plugin);
}

void
eina_plugin_cover_remove_backend(EinaPlugin *plugin, gchar *id)
{
	EinaCover *cover = eina_plugin_get_player_cover(plugin);
	if (cover == NULL)
	{
		gel_warn("Unable to access cover");
		return;
	}
	eina_cover_remove_backend(cover, id);
}
// --
// Artwork handling (cover replacement)
// --
EinaArtwork*
eina_plugin_get_artwork(EinaPlugin *plugin)
{
	return GEL_HUB_GET_ARTWORK(eina_plugin_get_hub(plugin));
}

void
eina_plugin_add_artwork_provider(EinaPlugin *plugin, gchar *id,
    EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel)
{
	EinaArtwork *artwork = eina_plugin_get_artwork(plugin);
	eina_artwork_add_provider(artwork, id, search, cancel, plugin);
}

void
eina_plugin_remove_artwork_provider(EinaPlugin *plugin, gchar *id)
{
	EinaArtwork *artwork = eina_plugin_get_artwork(plugin);
	eina_artwork_remove_provider(artwork, id);
}

// --
// LomoEvents handling
// --
void
eina_plugin_attach_events(EinaPlugin *self, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;

	va_start(p, self);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
			g_signal_connect(self->priv->lomo, signal,
			callback, self);
		signal = va_arg(p, gchar*);
	}
	va_end(p);
}

void
eina_plugin_deattach_events(EinaPlugin *self, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;

	va_start(p, self);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
			 g_signal_handlers_disconnect_by_func(self->priv->lomo, callback, self);
		signal = va_arg(p, gchar*);
	}
	va_end(p);
}

