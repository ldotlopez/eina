#define GEL_DOMAIN "Eina::Plugin"

#include <config.h>
#include <string.h> // strcmp
#include <gmodule.h>
#include <gel/gel.h>
#include <eina/eina-plugin.h>
#include <eina/dock2.h>
#include <eina/preferences2.h>

// --
// Functions needed to access private elements
// --
/*
inline const gchar *
eina_plagin_get_pathname(EinaPlagin *plugin)
{
	return (const gchar *) plugin->priv->pathname;
}

gboolean
eina_plagin_is_enabled(EinaPlagin *plugin)
{
	return plugin->priv->enabled;
}

LomoPlayer*
eina_plagin_get_lomo(EinaPlagin *plugin)
{
	return GEL_APP_GET_LOMO(gel_plugin_get_app(plugin));
}
*/

// --
// Utilities for plugins
// --
gchar *
eina_plagin_build_resource_path(EinaPlagin *plugin, gchar *resource)
{
	gchar *dirname, *ret;
	dirname = g_path_get_dirname(gel_plugin_get_pathname(plugin));

	ret = g_build_filename(dirname, resource, NULL);
	g_free(dirname);

	return ret;
}

gchar *
eina_plagin_build_userdir_path (EinaPlagin *self, gchar *path)
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
eina_plagin_get_player(EinaPlagin *self)
{
	GelApp     *app;

	if ((app = gel_plugin_get_app(self)) == NULL)
		return NULL;
	return GEL_APP_GET_PLAYER(app);
}

GtkWindow *
eina_plagin_get_main_window(EinaPlagin *self)
{
	EinaPlayer *player;

	if ((player = eina_plagin_get_player(self)) == NULL)
		return NULL;
	return eina_obj_get_typed(player, GTK_WINDOW, "main-window");
}

// --
// Dock management
// --
static EinaDock*
eina_plagin_get_dock(EinaPlagin *self)
{
	GelApp *app;
	if ((app = gel_plugin_get_app(self)) == NULL)
		return NULL;
	return GEL_APP_GET_DOCK(app);
}

gboolean eina_plagin_add_dock_widget(EinaPlagin *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	EinaDock *dock;

	if ((dock = eina_plagin_get_dock(self)) == NULL)
		return FALSE;

	return eina_dock_add_widget(dock, id, label, dock_widget);
}

gboolean eina_plagin_dock_remove_item(EinaPlagin *self, gchar *id)
{
	EinaDock *dock;

	if ((dock = eina_plagin_get_dock(self)) == NULL)
		return FALSE;

	return eina_dock_remove_widget(dock, id);
}

gboolean
eina_plagin_dock_switch(EinaPlagin *self, gchar *id)
{
	EinaDock *dock;

	if ((dock = eina_plagin_get_dock(self)) == NULL)
		return FALSE;

	return eina_dock_switch_widget(dock, id);
}

// --
// Settings management
// --
static EinaPreferencesDialog *
eina_plagin_get_preferences(EinaPlagin *self)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(self)) == NULL);
		return NULL;
	
	return GEL_APP_GET_PREFERENCES(app);
}

gboolean eina_plagin_add_configuration_widget
(EinaPlagin *plugin, GtkImage *icon, GtkLabel *label, GtkWidget *widget)
{
	EinaPreferencesDialog *prefs;

	if ((prefs = eina_plagin_get_preferences(plugin)) == NULL)
		return FALSE;

	eina_preferences_dialog_add_tab(prefs, icon, label, widget);
	return TRUE;
}

gboolean eina_plagin_remove_configuration_widget
(EinaPlagin *plugin,  GtkWidget *widget)
{
	EinaPreferencesDialog *prefs;

	if ((prefs = eina_plagin_get_preferences(plugin)) == NULL)
		return FALSE;

	eina_preferences_dialog_remove_tab(prefs, widget);
	return TRUE;
}

// --
// Artwork handling
// --
EinaArtwork*
eina_plagin_get_artwork(EinaPlagin *plugin)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(plugin)) == NULL)
		return NULL;

	return GEL_APP_GET_ARTWORK(app);
}

void
eina_plagin_add_artwork_provider(EinaPlagin *plugin, gchar *id,
    EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel)
{
	EinaArtwork *artwork;
	
	if ((artwork = eina_plagin_get_artwork(plugin)) == NULL)
		return;
	eina_artwork_add_provider(artwork, id, search, cancel, plugin);
}

void
eina_plagin_remove_artwork_provider(EinaPlagin *plugin, gchar *id)
{
	EinaArtwork *artwork;

	if ((artwork = eina_plagin_get_artwork(plugin)) == NULL)
		return;

	eina_artwork_remove_provider(artwork, id);
}

// --
// LomoEvents handling
// --
static LomoPlayer*
eina_plagin_get_lomo(EinaPlagin *self)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(self)) == NULL)
		return NULL;
	return GEL_APP_GET_LOMO(app);
}

void
eina_plagin_attach_events(EinaPlagin *self, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;
	LomoPlayer *lomo;

	if ((lomo = eina_plagin_get_lomo(self)) == NULL)
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
eina_plagin_deattach_events(EinaPlagin *self, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;
	LomoPlayer *lomo;

	if ((lomo = eina_plagin_get_lomo(self)) == NULL)
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

