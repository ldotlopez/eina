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
/*
 * We cant include gel/gel.h because GelPluginEngine and GelPlugin have
 * mutual deps. We use a dirty trick after G_BEGIN_DECLS
 */

G_BEGIN_DECLS

typedef struct _GelPlugin       GelPlugin;

#include <gel/gel-plugin-info.h>
#include <gel/gel-plugin-engine.h>
#include <gel/gel-misc.h>

#define GEL_PLUGIN(p)     ((GelPlugin *) p)
typedef enum {
	GEL_PLUGIN_NO_ERROR = 0,
	GEL_PLUGIN_ERROR_FINI_FAILED,
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

#ifdef GEL_COMPILATION
GelPlugin* gel_plugin_new (GelPluginEngine *engine, GelPluginInfo *info, GError **error);
gboolean   gel_plugin_free(GelPlugin *plugin, GError **error);

void     gel_plugin_add_reference(GelPlugin *plugin, GelPlugin *dependant);
void     gel_plugin_remove_reference(GelPlugin *plugin, GelPlugin *dependant);
#endif

const GelPluginInfo* gel_plugin_get_info(GelPlugin *plugin);
gpointer             gel_plugin_get_data(GelPlugin *plugin);
void                 gel_plugin_set_data(GelPlugin *plugin, gpointer data);
gpointer             gel_plugin_steal_data(GelPlugin *plugin);

gboolean gel_plugin_is_in_use(GelPlugin *plugin);
guint    gel_plugin_get_usage(GelPlugin *plugin);

GelPluginEngine* gel_plugin_get_engine  (GelPlugin *plugin);
const gchar* gel_plugin_stringify   (GelPlugin *plugin);
gboolean     gel_plugin_is_enabled  (GelPlugin *plugin);

const gchar* gel_plugin_get_pathname(GelPlugin *plugin);

GList * 
gel_plugin_get_resource_list(GelPlugin *plugin, GelResourceType type, gchar *resource); 
gchar * 
gel_plugin_get_resource(GelPlugin *plugin, GelResourceType type, gchar *resource); 

const GList* gel_plugin_get_dependants(GelPlugin *plugin);
gchar*       gel_plugin_stringify_dependants(GelPlugin *plugin);

const gchar* gel_plugin_get_lib_dir (GelPlugin *plugin);
const gchar* gel_plugin_get_data_dir(GelPlugin *plugin);

// Access to plugin's data if defined
#ifdef GEL_PLUGIN_DATA_TYPE
#define GEL_PLUGIN_DATA(p) ((GEL_PLUGIN_DATA_TYPE *) gel_plugin_get_data(GEL_PLUGIN(p)))
#endif


gpointer gel_plugin_get_application(GelPlugin *plugin);

#endif

