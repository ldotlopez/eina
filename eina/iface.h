#ifndef _EINA_IFACE
#define _EINA_IFACE

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <lomo/player.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>
#include <eina/fs.h>
#include <eina/eina-cover.h>

typedef struct _EinaIFace EinaIFace;

typedef struct _EinaPluginPrivateV2 EinaPluginPrivateV2;
typedef struct EinaPluginV2 {
	const gchar *name;       // "My cool plugin"
	const gchar *short_desc; // "This plugins makes Eina cooler
	const gchar *long_desc;  // "Blah blah blah..."
	const gchar *icon;       // "icon.png", relative path
	const gchar *author;     // "xuzo <xuzo@cuarentaydos.com>"
	const gchar *url;        // "http://eina.sourceforge.net"

	struct EinaPluginV2* (*init)(GelHub *app, EinaIFace *iface); // Init function
	gboolean             (*fini)(struct EinaPluginV2 *self);     // Exit function

	gpointer data; // Plugin's own data

	EinaPluginPrivateV2 *priv;
} EinaPluginV2;

typedef struct _EinaPluginPrivate EinaPluginPrivate;
typedef struct EinaPlugin
{
	const gchar *name;     // Name of the plugin. Eg "test"
	const gchar *provides; // "Capabilities" separated by comma. Eg. "playlist,cover"
	gpointer data;         // Field to store data used by plugin

	gboolean (*fini)(struct EinaPlugin *self); // Pointer to exit function

	GtkWidget *dock_widget;       // Widget to add to dock, NULL for none
	GtkWidget *dock_label_widget; // Widget to use as dock label, NULL for default

	GtkWidget  *conf_widget;      // Widget for configuration

	// 15 LomoPlayer signales, last argument of them (user_data) is this
	// struct: EinaPlugin
	void (*play);     // (LomoPlayer *self);
	void (*pause);    // (LomoPlayer *self);
	void (*stop);     // (LomoPlayer *self);
	void (*seek);     // (LomoPlayer *self, gint64 old, gint64 new);
	void (*mute);     // (LomoPlayer *self, gboolean mute);
	void (*add);      // (LomoPlayer *self, LomoStream *stream, gint pos);
	void (*del);      // (LomoPlayer *self, gint pos);
	void (*change);   // (LomoPlayer *self, gint from, gint to);
	void (*clear);    // (LomoPlayer *self);
	void (*repeat);   // (LomoPlayer *self, gboolean val);
	void (*random);   // (LomoPlayer *self, gboolean val);
	void (*eos);      // (LomoPlayer *self);
	void (*error);    // (LomoPlayer *self, GError *error);
	void (*tag);      // (LomoPlayer *self, LomoStream *stream, LomoTag tag);
	void (*all_tags); // (LomoPlayer *self, LomoStream *stream);

	EinaPluginPrivate *priv;
} EinaPlugin;

typedef EinaPlugin* (*EinaPluginInitFunc) (GelHub *app, EinaIFace *iface);
typedef gboolean    (*EinaPluginExitFunc) (EinaPlugin *self);

#define EINA_PLUGIN_FUNC G_MODULE_EXPORT

/*
 * Access to various resources from EinaPlugin, use defines in plugins code
 */

// Access GelHub
GelHub *
eina_iface_get_hub(EinaIFace *self);
#define EIN_PLUGIN_GEL_HUB(p) eina_iface_get_hub(EINA_PLUGIN_IFACE(p))

// Access LomoPlayer
LomoPlayer *
eina_plugin_get_lomo_player(EinaPlugin *plugin);
#define EINA_PLUGIN_LOMO_PLAYER(p) LOMO_PLAYER(eina_plugin_get_lomo_player(p))

// Access to EinaIFace
EinaIFace *
eina_plugin_get_iface(EinaPlugin *plugin);
#define EINA_PLUGIN_IFACE(p) eina_plugin_get_iface(p)

// Access to EinaCover object
EinaCover *
eina_iface_get_cover(EinaIFace *self);
#define EINA_PLUGIN_COVER(p) eina_iface_get_cover(EINA_PLUGIN_IFACE(p))

// Access to plugin's data
#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_GET_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) p->data)
#endif

/*
 * Functions to load/unload a complete plugin using EinaPlugin struct
 */
gboolean
eina_iface_plugin_register(EinaIFace *self, const EinaPlugin plugin);

gboolean
eina_iface_plugin_unregister(EinaIFace *self, gchar plugin_name);

gchar *
eina_iface_build_plugin_filename(gchar *plugin_name, gchar *filename);

gchar *
eina_iface_get_plugin_dir(gchar *plugin_name);

gchar *
eina_iface_plugin_resource_get_pathname(EinaPlugin *plugin, gchar *resource);

/*
 * Advanced functions for accesing some internals
 */
gboolean
eina_iface_dock_add(EinaIFace *self, gchar *id, GtkWidget *dock_widget, GtkWidget *label);

gboolean
eina_iface_dock_remove(EinaIFace *self, gchar *id);

gboolean
eina_iface_dock_switch(EinaIFace *self, gchar *id);

/*
 * Cover
 */
void eina_iface_cover_add_backend(EinaIFace *self, const gchar *name,
	EinaCoverBackendFunc callback, EinaCoverBackendCancelFunc cancel,
	gpointer data);
void eina_iface_cover_delete_backend(EinaIFace *self, const gchar *name);

#define eina_iface_verbose(...) _gel_debug(GEL_DEBUG_LEVEL_VERBOSE, __VA_ARGS__)
#define eina_iface_debug(...)   _gel_debug(GEL_DEBUG_LEVEL_DEBUG,   __VA_ARGS__)
#define eina_iface_info(...)    _gel_debug(GEL_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#define eina_iface_warn(...)    _gel_debug(GEL_DEBUG_LEVEL_WARN,    __VA_ARGS__)
#define eina_iface_error(...)   _gel_debug(GEL_DEBUG_LEVEL_ERROR,   __VA_ARGS__)

gboolean eina_iface_load_plugin(EinaIFace *self, gchar *plugin_name);
gboolean eina_iface_unload_plugin(EinaIFace *self, gchar *id);

/*
 * Create/free a EinaPlugin struct
 */
EinaPlugin *
eina_plugin_new(EinaIFace *iface,
	const gchar *name, const gchar *provides, gpointer data, EinaPluginExitFunc fini,
    GtkWidget *dock_widget, GtkWidget *dock_label, GtkWidget *conf_widget);
void
eina_plugin_free(EinaPlugin *self);

#endif /* _EINA_IFACE */

