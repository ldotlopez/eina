/*
 * gel/gel-app.h
 *
 * Copyright (C) 2004-2010 Eina
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

#ifndef _GEL_APP_H
#define _GEL_APP_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GelApp GelApp;
#include <gel/gel-plugin.h>
#include <gel/gel-plugin-info.h>

#define GEL_TYPE_APP gel_app_get_type()

#define GEL_APP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_TYPE_APP, GelApp))

#define GEL_APP_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GEL_TYPE_APP, GelAppClass))

#define GEL_IS_APP(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_TYPE_APP))

#define GEL_IS_APP_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_TYPE_APP))

#define GEL_APP_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_TYPE_APP, GelAppClass))

typedef struct _GelAppPrivate GelAppPriv;

struct _GelApp {
	GObject parent;
	GelAppPriv *priv;
};

typedef struct {
	GObjectClass parent_class;
	void (*plugin_init)   (GelApp *self, GelPlugin *plugin);
	void (*plugin_fini)   (GelApp *self, GelPlugin *plugin);
} GelAppClass;
typedef void (*GelAppDisposeFunc) (GelApp *self, gpointer data);

enum {
	GEL_APP_NO_ERROR = 0,

	GEL_PLUGIN_INFO_NOT_FOUND,
	GEL_APP_MISSING_PLUGIN_DEPS,
	GEL_APP_PLUGIN_HAS_RDEPS,

	GEL_APP_ERROR_GENERIC,
	GEL_APP_ERROR_INVALID_ARGUMENTS,
	GEL_APP_PLUGIN_NOT_FOUND,
	GEL_APP_PLUGIN_DEP_NOT_FOUND,
	GEL_APP_ERROR_PLUGIN_IN_USE,

	GEL_APP_ERROR_PLUGIN_ALREADY_LOADED_BY_NAME
	/*
	GEL_APP_NO_OWNED_PLUGIN,
	GEL_APP_PLUGIN_STILL_ENABLED,
	GEL_APP_CANNOT_OPEN_SHARED_OBJECT,
	GEL_APP_SYMBOL_NOT_FOUND
	*/
};

GType gel_app_get_type (void);

GelApp* gel_app_new (void);

void gel_app_set_dispose_callback(GelApp *self, GelAppDisposeFunc callback, gpointer user_data);

GelPlugin *gel_app_load_plugin_by_name    (GelApp *self, gchar *name,     GError **error);
GelPlugin *gel_app_load_plugin            (GelApp *self, GelPluginInfo *info, GError **error);

GList     *gel_app_get_plugins(GelApp *self);
GelPlugin *gel_app_get_plugin (GelApp *self, GelPluginInfo *info);
GelPlugin *gel_app_get_plugin_by_name(GelApp *self, gchar *name);

void   gel_app_scan_plugins(GelApp *app);
GList *gel_app_query_plugins(GelApp *app);

gboolean   gel_app_unload_plugin(GelApp *self, GelPlugin *plugin, GError **error);
void       gel_app_purge(GelApp *self);

void     gel_app_shared_free(GelApp *self, gchar *name);
gboolean gel_app_shared_set(GelApp *self, gchar *name, gpointer data);
gpointer gel_app_shared_get(GelApp *self, gchar *name);

#if (defined GEL_COMPILATION) && (defined _GEL_PLUGIN_H)
void gel_app_priv_run_init(GelApp *self, GelPlugin *plugin);
void gel_app_priv_run_fini(GelApp *self, GelPlugin *plugin);
#endif

G_END_DECLS

#endif /* _GEL_APP */
