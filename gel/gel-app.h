/*
 * gel/gel-app.h
 *
 * Copyright (C) 2004-2009 Eina
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

#ifndef _GEL_APP
#define _GEL_APP

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GelApp GelApp;
#include <gel/gel-plugin.h>

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
	void (*plugin_load)   (GelApp *self, GelPlugin *plugin);
	void (*plugin_unload) (GelApp *self, GelPlugin *plugin);
	void (*plugin_init)   (GelApp *self, GelPlugin *plugin);
	void (*plugin_fini)   (GelApp *self, GelPlugin *plugin);
	// void (*plugin_ref)    (GelApp *self, const gchar *name, guint refs);
	// void (*plugin_unref)  (GelApp *self, const gchar *name, guint refs);
} GelAppClass;
typedef void (*GelAppDisposeFunc) (GelApp *self, gpointer data);

enum {
	GEL_APP_NO_ERROR = 0,
	GEL_APP_NO_OWNED_PLUGIN,
	GEL_APP_PLUGIN_STILL_ENABLED,
	// GEL_APP_CANNOT_OPEN_SHARED_OBJECT,
	// GEL_APP_SYMBOL_NOT_FOUND
};

GType gel_app_get_type (void);

GelApp* gel_app_new (void);

void gel_app_set_dispose_callback(GelApp *self, GelAppDisposeFunc callback, gpointer user_data);

GelPlugin *gel_app_load_plugin (GelApp *self, gchar *pathname, GError **error);
GelPlugin *gel_app_load_plugin_by_name(GelApp *self, gchar *plugin_name);
GelPlugin *gel_app_load_buildin(GelApp *self, gchar *symbol, GError **error);
GelPlugin *gel_app_load_plugin_full(GelApp *self, gchar *pathname, gchar *symbol, GError **error);

gboolean   gel_app_unload_plugin(GelApp *self, GelPlugin *plugin, GError **error);
gboolean   gel_app_unload_plugin_by_name(GelApp *self, gchar *plugin_name);

gboolean gel_app_init_plugin(GelApp *self, GelPlugin *plugin, GError **error);
gboolean gel_app_fini_plugin(GelApp *self, GelPlugin *plugin, GError **error);

GList *gel_app_query_plugins(GelApp *self);
GList *gel_app_query_paths(GelApp *self);

GelPlugin *gel_app_query_plugin(GelApp *self, gchar *pathname, gchar *symbol);
GelPlugin *gel_app_query_plugin_by_pathname(GelApp *self, gchar *pathname);
GelPlugin *gel_app_query_plugin_by_name    (GelApp *self, gchar *name);

#define gel_app_plugin_is_loaded_by_name(self,pathname) \
	(gel_app_query_plugin_by_name(self,pathname) != NULL)

gboolean gel_app_shared_register  (GelApp *self, gchar *name, gsize size);
gboolean gel_app_shared_unregister(GelApp *self, gchar *name);

gboolean gel_app_shared_set(GelApp *self, gchar *name, gpointer data);
gpointer gel_app_shared_get(GelApp *self, gchar *name);

G_END_DECLS

#endif /* _GEL_APP */
