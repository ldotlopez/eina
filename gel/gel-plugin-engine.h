/*
 * gel/gel-plugin-engine.h
 *
 * Copyright (C) 2004-2011 Eina
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

#ifndef _GEL_PLUGIN_ENGINE_H
#define _GEL_PLUGIN_ENGINE_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _GelPluginEngine GelPluginEngine;
#include <gel/gel-plugin.h>

#define GEL_TYPE_PLUGIN_ENGINE gel_plugin_engine_get_type()

#define GEL_PLUGIN_ENGINE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_TYPE_PLUGIN_ENGINE, GelPluginEngine))

#define GEL_PLUGIN_ENGINE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GEL_TYPE_PLUGIN_ENGINE, GelPluginEngineClass))

#define GEL_IS_PLUGIN_ENGINE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_TYPE_PLUGIN_ENGINE))

#define GEL_IS_PLUGIN_ENGINE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_TYPE_PLUGIN_ENGINE))

#define GEL_PLUGIN_ENGINE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_TYPE_PLUGIN_ENGINE, GelPluginEngineClass))

typedef struct _GelPluginEnginePrivate GelPluginEnginePriv;

typedef enum {
	GEL_PLUGIN_ENGINE_NO_ERROR = 0,
	GEL_PLUGIN_ENGINE_MISSING_PLUGIN_DEPS,
	GEL_PLUGIN_ENGINE_ERROR_INVALID_ARGUMENTS,
	GEL_PLUGIN_ENGINE_INFO_NOT_FOUND
} GelPluginEngineError;

struct _GelPluginEngine {
	GObject parent;
	GelPluginEnginePriv *priv;
};

typedef struct {
	GObjectClass parent_class;
	void (*plugin_init)   (GelPluginEngine *self, GelPlugin *plugin);
	void (*plugin_fini)   (GelPluginEngine *self, GelPlugin *plugin);
} GelPluginEngineClass;
typedef void (*GelPluginEngineDisposeFunc) (GelPluginEngine *self, gpointer data);

GType gel_plugin_engine_get_type (void);

GelPluginEngine* gel_plugin_engine_new(gpointer application);
GelPluginEngine* gel_plugin_engine_get_default(void);

void gel_plugin_engine_set_dispose_callback(GelPluginEngine *self, GelPluginEngineDisposeFunc callback, gpointer user_data);

GelPlugin *gel_plugin_engine_load_plugin_by_name    (GelPluginEngine *self, gchar *name,     GError **error);
GelPlugin *gel_plugin_engine_load_plugin_by_pathname(GelPluginEngine *self, gchar *pathname, GError **error);
GelPlugin *gel_plugin_engine_load_plugin            (GelPluginEngine *self, GelPluginInfo *info, GError **error);

GList     *gel_plugin_engine_get_plugins(GelPluginEngine *self);
GelPlugin *gel_plugin_engine_get_plugin (GelPluginEngine *self, GelPluginInfo *info);
GelPlugin *gel_plugin_engine_get_plugin_by_name(GelPluginEngine *self, gchar *name);

void       gel_plugin_engine_scan_plugins(GelPluginEngine *self);
GList     *gel_plugin_engine_query_plugins(GelPluginEngine *self);

gboolean   gel_plugin_engine_unload_plugin(GelPluginEngine *self, GelPlugin *plugin, GError **error);
void       gel_plugin_engine_purge(GelPluginEngine *self);

gint    *gel_plugin_engine_get_argc(GelPluginEngine *self);
gchar ***gel_plugin_engine_get_argv(GelPluginEngine *self);

gpointer gel_plugin_engine_get_application(GelPluginEngine *self);

#if (defined GEL_COMPILATION) && (defined _GEL_PLUGIN_H)
void gel_plugin_engine_priv_run_init(GelPluginEngine *self, GelPlugin *plugin);
void gel_plugin_engine_priv_run_fini(GelPluginEngine *self, GelPlugin *plugin);
#endif

G_END_DECLS

#endif /* _GEL_PLUGIN_ENGINE */
