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

typedef struct _GelPluginInfo   GelPluginInfo;
typedef struct _GelPlugin       GelPlugin;
typedef struct _GelPluginExtend GelPluginExtend;
#include <gel/gel-app.h>
#include <gel/gel-misc.h>

#define GEL_PLUGIN(p)     ((GelPlugin *) p)
#define GEL_PLUGIN_SERIAL 2009042401
#define GEL_PLUGIN_NO_DEPS "\1"
typedef enum {
	GEL_PLUGIN_NO_ERROR = 0,

	GEL_PLUGIN_INFO_NULL_FILENAME,
	GEL_PLUGIN_INFO_HAS_NO_GROUP,
	GEL_PLUGIN_INFO_MISSING_KEY,
	GEL_PLUGIN_INFO_CANNOT_OPEN_MODULE,
	GEL_PLUGIN_INFO_SYMBOL_NOT_FOUND,

	GEL_PLUGIN_ERROR_CANNOT_INIT_DEP,

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

struct _GelPluginInfo {
	gchar *dirname, *pathname;
	gchar *version, *depends, *author, *url;
	gchar *short_desc, *long_desc, *icon_pathname;
	gchar *name;
};

#ifdef GEL_COMPILATION
GelPluginInfo *gel_plugin_info_new(const gchar *filename, const gchar *name, GError **error);
void           gel_plugin_info_free(GelPluginInfo *pinfo);
void           gel_plugin_info_copy(GelPluginInfo *src, GelPluginInfo *dst);
gboolean       gel_plugin_info_cmp(GelPluginInfo *a, GelPluginInfo *b);

GelPlugin* gel_plugin_new (GelApp *app, GelPluginInfo *info, GError **error);
gboolean   gel_plugin_free(GelPlugin *plugin, GError **error);

void     gel_plugin_add_reference(GelPlugin *plugin, GelPlugin *dependant);
void     gel_plugin_remove_reference(GelPlugin *plugin, GelPlugin *dependant);
#endif

GelPluginInfo* gel_plugin_get_info(GelPlugin *plugin);
gpointer       gel_plugin_get_data(GelPlugin *plugin);
void           gel_plugin_set_data(GelPlugin *plugin, gpointer data);

gboolean gel_plugin_is_in_use(GelPlugin *plugin);
guint    gel_plugin_get_usage(GelPlugin *plugin);

GelApp*      gel_plugin_get_app     (GelPlugin *plugin);
const gchar* gel_plugin_stringify   (GelPlugin *plugin);
gboolean     gel_plugin_is_enabled  (GelPlugin *plugin);
const gchar* gel_plugin_get_pathname(GelPlugin *plugin);

GList * 
gel_plugin_get_resource_list(GelPlugin *plugin, GelResourceType type, gchar *resource); 
gchar * 
gel_plugin_get_resource(GelPlugin *plugin, GelResourceType type, gchar *resource); 

const GList* gel_plugin_get_dependants(GelPlugin *plugin);
gchar*       gel_plugin_stringify_dependants(GelPlugin *plugin);

// Access to plugin's data if defined
#ifdef GEL_PLUGIN_DATA_TYPE
#define GEL_PLUGIN_DATA(p) ((GEL_PLUGIN_DATA_TYPE *) gel_plugin_get_data(GEL_PLUGIN(p)))
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
