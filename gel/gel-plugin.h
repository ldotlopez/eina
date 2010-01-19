/*
 * gel/gel-plugin.h
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

#ifndef _GEL_PLUGIN_H
#define _GEL_PLUGIN_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GelPlugin       GelPlugin;
typedef struct _GelPluginExtend GelPluginExtend;
#include <gel/gel-app.h>
#include <gel/gel-misc.h>

#define GEL_PLUGIN(p)     ((GelPlugin *) p)
#define GEL_PLUGIN_SERIAL 2009042401
#define GEL_PLUGIN_NO_DEPS "\1"
typedef enum {
	GEL_PLUGIN_NO_ERROR = 0,
	GEL_PLUGIN_ERROR_NOT_SUPPORTED,
	GEL_PLUGIN_ERROR_MODULE_NOT_LOADABLE,
	GEL_PLUGIN_ERROR_SYMBOL_NOT_FOUND,
	GEL_PLUGIN_ERROR_INVALID_SERIAL,
	GEL_PLUGIN_ERROR_IN_USE,
	GEL_PLUGIN_ERROR_IS_ENABLED,
	GEL_PLUGIN_ERROR_LOCKED,
	GEL_PLUGIN_ERROR_ALREADY_INITIALIZED,
	GEL_PLUGIN_ERROR_NO_INIT_HOOK,
	GEL_PLUGIN_ERROR_UNKNOW,
	GEL_PLUGIN_ERROR_NOT_INITIALIZED,
} GelPluginError;

typedef struct _GelPluginPrivate GelPluginPrivate;
struct _GelPlugin {
	guint serial;            // GEL_PLUGIN_SERIAL
	const gchar *name;       // "My cool plugin"
	const gchar *version;    // "1.0.0"
	const gchar *depends;    // "foo,bar,baz" or NULL
	const gchar *author;     // "me <me@company.com>"
	const gchar *url;        // "http://www.company.com"
	const gchar *short_desc; // "This plugins makes my app cooler"
	const gchar *long_desc;  // "Blah blah blah..."
	const gchar *icon;       // "icon.png", relative path

	gboolean (*init) (GelApp *app, struct _GelPlugin *plugin, GError **error); // Init function
	gboolean (*fini) (GelApp *app, struct _GelPlugin *plugin, GError **error); // Exit function

	gpointer data;             // Plugin's own data

	GelPluginPrivate *priv;   // GelPlugin system private data, must be NULL
	GelPluginExtend  *extend; // Reserved for extensions, must be NULL
};

#ifdef GEL_COMPILATION
GelPlugin* gel_plugin_new (GelApp *app, gchar *pathname, gchar *symbol, GError **error);
gboolean   gel_plugin_free(GelPlugin *plugin, GError **error);

gboolean gel_plugin_init(GelPlugin *plugin, GError **error);
gboolean gel_plugin_fini(GelPlugin *plugin, GError **error);

void     gel_plugin_add_reference(GelPlugin *plugin, GelPlugin *dependant);
void     gel_plugin_remove_reference(GelPlugin *plugin, GelPlugin *dependant);
#endif

gboolean gel_plugin_is_in_use(GelPlugin *plugin);
guint    gel_plugin_get_usage(GelPlugin *plugin);

gboolean gel_plugin_is_locked(GelPlugin *plugin);
void     gel_plugin_add_lock(GelPlugin *plugin);
void     gel_plugin_remove_lock(GelPlugin *plugin);

GelApp*      gel_plugin_get_app     (GelPlugin *plugin);
const gchar* gel_plugin_stringify   (GelPlugin *plugin);
gboolean     gel_plugin_is_enabled  (GelPlugin *plugin);
const gchar* gel_plugin_get_pathname(GelPlugin *plugin);

GList * 
gel_plugin_get_resource_list(GelPlugin *plugin, GelResourceType type, gchar *resource); 
gchar * 
gel_plugin_get_resource(GelPlugin *plugin, GelResourceType type, gchar *resource); 

#if 0
gchar*       gel_plugin_build_resource_path(GelPlugin *plugin, gchar *resource_path);
#endif
const GList* gel_plugin_get_dependants(GelPlugin *plugin);
gchar*       gel_plugin_stringify_dependants(GelPlugin *plugin);

// Access to plugin's data if defined
#ifdef GEL_PLUGIN_DATA_TYPE
#define GEL_PLUGIN_DATA(p) ((GEL_PLUGIN_DATA_TYPE *) GEL_PLUGIN(p)->data)
#endif

// Utils
gint    gel_plugin_compare_by_name (GelPlugin *a, GelPlugin *b);
gint    gel_plugin_compare_by_usage(GelPlugin *a, GelPlugin *b);

gchar*  gel_plugin_util_symbol_from_pathname(gchar *pathname);
#define gel_plugin_util_symbol_from_name(name) g_strconcat((name?name:""), "_plugin", NULL)
gchar*  gel_plugin_util_symbol_from_pathname(gchar *pathname);
#define gel_plugin_util_stringify(pathname,name) g_build_filename((pathname?pathname:"<NULL>"),name?name:"<NULL>",NULL)
gchar*  gel_plugin_util_stringify_for_pathname(gchar *pathname);
gchar*  gel_plugin_util_join_list(gchar *separator, GList *plugin_list);

#endif
