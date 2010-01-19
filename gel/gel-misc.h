/*
 * gel/gel-misc.h
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

#ifndef _GEL_MISC_H_
#define _GEL_MISC_H_

#include <glib.h>

G_BEGIN_DECLS

// --
// Types
// --
typedef enum GelDebugLevel
{
	GEL_DEBUG_LEVEL_SEVERE  = 0,
	GEL_DEBUG_LEVEL_ERROR   = 1,
	GEL_DEBUG_LEVEL_WARN    = 2,
	GEL_DEBUG_LEVEL_INFO    = 3,
	GEL_DEBUG_LEVEL_DEBUG   = 4,
	GEL_N_DEBUG_LEVELS      = 5
} GelDebugLevel;

enum {
	GEL_BASENAME,
	GEL_DIRNAME,
	GEL_PATHNAME,
	GEL_N_PATH_STRINGS
};

typedef enum {
	GEL_RESOURCE_UI,     // ui/
	GEL_RESOURCE_IMAGE,  // pixmaps/
	GEL_RESOURCE_SHARED, // lib/
	GEL_RESOURCE_OTHER,  // "./"

	GEL_N_RESOURCES
} GelResourceType;

typedef enum GelFileLoadCode {
	GEL_FILE_LOAD_CODE_OK,
	GEL_FILE_LOAD_CODE_NOT_FOUND,
	GEL_FILE_LOAD_CODE_CANNOT_LOAD,
} GelFileLoadCode;

typedef gchar*   (*GelListPrintfFunc)(const gpointer data);
typedef gboolean (*GelFilterFunc)    (const gpointer data, gpointer user_data);

#define GEL_AUTO_QUARK_FUNC(name) \
	static GQuark name ## _quark(void) \
	{ \
		static GQuark ret = 0; \
		if (ret == 0) \
			ret = g_quark_from_static_string(G_STRINGIFY(name)); \
		return ret; \
	}

// --
// Initialization and finilization functions
// --
void
gel_init(gchar *app_name, gchar *lib_dir, gchar *data_dir);

const gchar*
gel_get_package_name(void);

const gchar*
gel_get_package_lib_dir(void);

const gchar*
gel_get_package_data_dir(void);

// --
// Utilities for GList/GSList
// --
gchar **
gel_list_to_strv(GList *list, gboolean copy);

GList *
gel_strv_to_list(gchar **strv, gboolean copy);

gchar *
gel_list_join(const gchar *separator, GList *list);

// GSList are compatibles
#define gel_slist_deep_free(list,callback) gel_list_deep_free((GList*)list,callback)
#define gel_slist_deep_free_with_data(list,callback,user_data) gel_list_deep_free_with_data((GList*)list,callback,user_data)

#define gel_list_deep_free(list,callback) gel_list_deep_free_with_data(list,(GFunc)callback,NULL)
#define gel_list_deep_free_with_data(list,callback,user_data) \
	do { \
		if (list != NULL) { \
			g_list_foreach(list, callback, user_data); \
			g_list_free(list); \
		} \
	} while (0)


// Filter: no data copied, its shared
#define gel_list_filter(input,callback,user_data) gel_slist_filter((GSList*)input,callback,user_data); // Compatible
GSList*
gel_slist_filter(GSList *input, GelFilterFunc callback, gpointer user_data);

void
gel_slist_differential_free(GSList *list, GSList *hold, GCompareFunc compare, GFunc callback, gpointer user_data);

#define gel_slist_printf(list,format,func) gel_list_printf((GList*) list, format, func)
void
gel_list_printf(GList *list, gchar *format, GelListPrintfFunc stringify_func);

// --
// App resources functions
// --
const gchar *
gel_resource_type_get_env(GelResourceType type);
gchar *
gel_resource_type_get_user_dir(GelResourceType type);
gchar *
gel_resource_type_get_system_dir(GelResourceType type);

gchar *
gel_resource_locate(GelResourceType type, gchar *resource);


gchar *
gel_app_userdir_get_pathname(gchar *appname, gchar *filename, gboolean create_parents, gint mode);
gchar *
gel_app_build_config_filename(gchar *name, gboolean create_path, gint dir_mode, GError **error);

// --
// File system utilities
// --
GList *
gel_dir_read(gchar *path, gboolean absolute, GError **error);

gchar **
gel_file_strings(gchar *pathname);

// --
// Timeout functions
// --
guint
gel_run_once_on_idle(GSourceFunc callback, gpointer data, GDestroyNotify destroy);

guint
gel_run_once_on_timeout(guint interval, GSourceFunc callback, gpointer data, GDestroyNotify destroy);


// --
// Pointers safe free functions
// --
#define gel_free_and_invalidate(obj,value,func) \
	do { \
		if (obj != value)  \
		{ \
			func(obj); \
			obj = value; \
		} \
	} while(0)
#define gel_free_and_invalidate_with_args(obj,value,func,...) \
	do { \
		if (obj != value)  \
		{ \
			func(obj, __VA_ARGS__); \
			obj = value; \
		} \
	} while(0)


// --
// Totally misc functions
// --
#define gel_str_or_text(str, text) (str ? str : text)

// --
// Debug functions
// --
typedef void (*GelDebugHandler) (GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer, gpointer data);

GelDebugLevel
gel_get_debug_level(void);
void
gel_set_debug_level(GelDebugLevel level);

void gel_debug_add_handler   (GelDebugHandler func, gpointer data);
void gel_debug_remove_handler(GelDebugHandler func);


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define _gel_debug(level,...) gel_debug_real (GEL_DOMAIN, level, __func__, __FILE__, __LINE__, __VA_ARGS__)
#elif defined(__GNUC__) && __GNUC__ >= 3
#define _gel_debug(level,...) gel_debug_real (GEL_DOMAIN, level, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define _gel_debug
#endif

// #define gel_verbose(...)    _gel_debug(GEL_DEBUG_LEVEL_VERBOSE, __VA_ARGS__)
#define gel_debug(...)	    _gel_debug(GEL_DEBUG_LEVEL_DEBUG,   __VA_ARGS__)
#define gel_info(...)       _gel_debug(GEL_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#define gel_warn(...)       _gel_debug(GEL_DEBUG_LEVEL_WARN,    __VA_ARGS__)
#define gel_error(...)      _gel_debug(GEL_DEBUG_LEVEL_ERROR,   __VA_ARGS__)
#define gel_apocalipse(...) _gel_debug(GEL_DEBUG_LEVEL_SEVERE,  __VA_ARGS__)
#define gel_implement(...)  gel_warn("IMPLEMENT-ME -- " __VA_ARGS__)
#define gel_fix(...)        gel_warn("FIX-ME -- " __VA_ARGS__)

void
gel_debug_real  (const gchar *domain, GelDebugLevel level, const char *func, const char *file, int line, const char *format, ...);

G_END_DECLS

#endif // _GEL_MISC_H

