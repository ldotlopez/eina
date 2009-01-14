#define GEL_DOMAIN "Eina::Plugin"

#include <config.h>
#include <string.h> // strcmp
#include <gmodule.h>
#include <gel/gel.h>
#include <eina/eina-plugin.h>
#include <eina/dock.h>
#include <eina/preferences.h>

// --
// Functions needed to access private elements
// --
/*
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
*/

// --
// Utilities for plugins
// --
gchar *
eina_plugin_build_resource_path(EinaPlugin *plugin, gchar *resource)
{
	gchar *dirname, *ret;
	dirname = g_path_get_dirname(gel_plugin_get_pathname(plugin));

	ret = g_build_filename(dirname, resource, NULL);
	g_free(dirname);

	return ret;
}

gchar *
eina_plugin_build_userdir_path (EinaPlugin *self, gchar *path)
{
	gchar *dirname = g_path_get_dirname(gel_plugin_get_pathname(self));
	gchar *bname   = g_path_get_basename(dirname);
	gchar *ret     = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, bname, path, NULL);
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
	GelApp     *app;

	if ((app = gel_plugin_get_app(self)) == NULL)
		return NULL;
	return GEL_APP_GET_PLAYER(app);
}

GtkWindow *
eina_plugin_get_main_window(EinaPlugin *self)
{
	EinaPlayer *player;

	if ((player = eina_plugin_get_player(self)) == NULL)
		return NULL;
	return eina_obj_get_typed(player, GTK_WINDOW, "main-window");
}

// --
// Dock management
// --
static EinaDock*
eina_plugin_get_dock(EinaPlugin *self)
{
	GelApp *app;
	if ((app = gel_plugin_get_app(self)) == NULL)
		return NULL;
	return GEL_APP_GET_DOCK(app);
}

gboolean eina_plugin_add_dock_widget(EinaPlugin *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	EinaDock *dock;

	if ((dock = eina_plugin_get_dock(self)) == NULL)
		return FALSE;

	return eina_dock_add_widget(dock, id, label, dock_widget);
}

gboolean eina_plugin_dock_remove_item(EinaPlugin *self, gchar *id)
{
	EinaDock *dock;

	if ((dock = eina_plugin_get_dock(self)) == NULL)
		return FALSE;

	return eina_dock_remove_widget(dock, id);
}

gboolean
eina_plugin_dock_switch(EinaPlugin *self, gchar *id)
{
	EinaDock *dock;

	if ((dock = eina_plugin_get_dock(self)) == NULL)
		return FALSE;

	return eina_dock_switch_widget(dock, id);
}

// --
// Settings management
// --
static EinaPreferencesDialog *
eina_plugin_get_preferences(EinaPlugin *self)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(self)) == NULL);
		return NULL;
	
	return GEL_APP_GET_PREFERENCES(app);
}

gboolean eina_plugin_add_configuration_widget
(EinaPlugin *plugin, GtkImage *icon, GtkLabel *label, GtkWidget *widget)
{
	EinaPreferencesDialog *prefs;

	if ((prefs = eina_plugin_get_preferences(plugin)) == NULL)
		return FALSE;

	eina_preferences_dialog_add_tab(prefs, icon, label, widget);
	return TRUE;
}

gboolean eina_plugin_remove_configuration_widget
(EinaPlugin *plugin,  GtkWidget *widget)
{
	EinaPreferencesDialog *prefs;

	if ((prefs = eina_plugin_get_preferences(plugin)) == NULL)
		return FALSE;

	eina_preferences_dialog_remove_tab(prefs, widget);
	return TRUE;
}

// --
// Artwork handling
// --
EinaArtwork*
eina_plugin_get_artwork(EinaPlugin *plugin)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(plugin)) == NULL)
		return NULL;

	return GEL_APP_GET_ARTWORK(app);
}

void
eina_plugin_add_artwork_provider(EinaPlugin *plugin, gchar *id,
    EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel)
{
	EinaArtwork *artwork;
	
	if ((artwork = eina_plugin_get_artwork(plugin)) == NULL)
		return;
	eina_artwork_add_provider(artwork, id, search, cancel, plugin);
}

void
eina_plugin_remove_artwork_provider(EinaPlugin *plugin, gchar *id)
{
	EinaArtwork *artwork;

	if ((artwork = eina_plugin_get_artwork(plugin)) == NULL)
		return;

	eina_artwork_remove_provider(artwork, id);
}

// --
// LomoEvents handling
// --
LomoPlayer*
eina_plugin_get_lomo(EinaPlugin *self)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(self)) == NULL)
		return NULL;
	return GEL_APP_GET_LOMO(app);
}

void
eina_plugin_attach_events(EinaPlugin *self, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;
	LomoPlayer *lomo;

	if ((lomo = eina_plugin_get_lomo(self)) == NULL)
		return;

	va_start(p, self);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
			g_signal_connect(lomo, signal,
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
	LomoPlayer *lomo;

	if ((lomo = eina_plugin_get_lomo(self)) == NULL)
		return;

	va_start(p, self);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
			 g_signal_handlers_disconnect_by_func(lomo, callback, self);
		signal = va_arg(p, gchar*);
	}
	va_end(p);
}
