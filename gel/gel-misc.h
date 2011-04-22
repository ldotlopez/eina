/*
 * gel/gel-misc.h
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

#ifndef __GEL_MISC_H__
#define __GEL_MISC_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Data types                                                                */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * GelDebugLevel:
 * @GEL_DEBUG_LEVEL_SEVERE: Severe error code
 * @GEL_DEBUG_LEVEL_ERROR: 'Normal' error code
 * @GEL_DEBUG_LEVEL_WARN: Warning code
 * @GEL_DEBUG_LEVEL_INFO: Detailed information
 * @GEL_DEBUG_LEVEL_DEBUG: Insane debug
 *
 * Codes for the gel debug system 
 *
 * Deprecated: 1.2: Use standard glib log system
 **/
typedef enum GelDebugLevel
{
	GEL_DEBUG_LEVEL_SEVERE  = 0,
	GEL_DEBUG_LEVEL_ERROR   = 1,
	GEL_DEBUG_LEVEL_WARN    = 2,
	GEL_DEBUG_LEVEL_INFO    = 3,
	GEL_DEBUG_LEVEL_DEBUG   = 4,
	GEL_N_DEBUG_LEVELS      = 5
} GelDebugLevel;

/**
 * GelPathComponent:
 *
 * @GEL_PATH_COMPONENT_BASENAME: Basename of a path string
 * @GEL_PATH_COMPONENT_DIRNAME: Dirname of a path string
 * @GEL_PATH_COMPONENT_PATHNAME: Full path string
 * @GEL_PATH_N_COMPONENTS: Helper define
 *
 * Used for access returned data from %gel_file_strings
 **/
typedef enum {
	GEL_PATH_COMPONENT_BASENAME,
	GEL_PATH_COMPONENT_DIRNAME,
	GEL_PATH_COMPONENT_PATHNAME,
	GEL_PATH_N_COMPONENTS
} GelPathComponent;

/**
 * GelResourceType:
 *
 * @GEL_RESOURCE_TYPE_UI: A .ui file, a UI definition understandable by GtkBuilder
 * @GEL_RESOURCE_TYPE_IMAGE: An image in any format loadable by Gtk+
 * @GEL_RESOURCE_TYPE_SHARED: A shared object (.so, .dylib, .dll)
 * @GEL_RESOURCE_OTHER: Other types
 *
 * GelResourceType describes which type of file to locate, each type, except
 * @GEL_RESOURCE_OTHER, is searched in specific locations, where
 * @GEL_RESOURCE_OTHER is searched at '.'
 *
 * For specific information about search paths you should to read the code of
 * gel/gel-misc.c.
 **/
typedef enum {
	GEL_RESOURCE_TYPE_UI,     // ui/
	GEL_RESOURCE_TYPE_IMAGE,  // pixmaps/
	GEL_RESOURCE_TYPE_SHARED, // lib/
	GEL_RESOURCE_TYPE_OTHER,  // "./"

	GEL_RESOURCE_N_TYPES
} GelResourceType;

/**
 * GelSignalDef:
 * @object: Name of the object
 * @signal: Signal name
 * @callback: #GCallback to connect
 * @data: data to pass to callback or %NULL
 * @swapped: Whatever to use swapped version of g_signal_connect()
 *
 * Defines a signal
 */
typedef struct {
	gchar *object, *signal;
	GCallback callback;
	gpointer data;
	gboolean swapped;
} GelSignalDef;

/**
 * GelPrintFunc:
 *
 * @data: (transfer none): gpointer holind data to dump
 *
 * Converts @data to printable strings
 *
 * Returns: (transfer full): Data in a printable form
 **/
typedef gchar*   (*GelPrintFunc)(const gpointer data);

/**
 * GelFilterFunc:
 *
 * @data: Data to filter or not
 * @user_data: User data
 *
 * Filters out (or not) @data from a larger set of values
 *
 * Returns: %TRUE if data is accepted (NOT sure about this, refer to caller
 *          function documentation)
 **/
typedef gboolean (*GelFilterFunc)    (const gpointer data, gpointer user_data);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Defines                                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * GEL_DEFINE_QUARK_FUNC:
 *
 * @name: Name for use in the quark function, use 'foo' for quark_foo()
 *
 * Defines a quark function
 **/
#define GEL_DEFINE_QUARK_FUNC(name) \
	static GQuark name ## _quark(void) \
	{ \
		static GQuark ret = 0; \
		if (ret == 0) \
			ret = g_quark_from_static_string(G_STRINGIFY(name)); \
		return ret; \
	}

/**
 * GEL_DEFINE_WEAK_REF_CALLBACK:
 *
 * @name: Name for use in the weak ref callback, use 'foo' for foo_weak_ref_cb
 *
 * Defines a weak ref function for use with g_object_weak_ref
 */
#define GEL_DEFINE_WEAK_REF_CALLBACK(name) \
	static void \
	name ## _weak_ref_cb (gpointer data, GObject *_object) \
	{ \
	    g_warning("Protected object %p is begin destroyed. There is a bug somewhere, set a breakpoint on %s", _object, G_STRINGIFY(name)  "_weak_ref_cb"); \
	}

// --
// Initialization functions
// --

void
gel_init(const gchar *package_name, const gchar *lib_dir, const gchar *data_dir);

const gchar*
gel_get_package_name(void);

const gchar*
gel_get_package_lib_dir(void);

const gchar*
gel_get_package_data_dir(void);

#define gel_propagate_error_or_warm(dst,src,...) \
	G_STMT_START {                   \
		if (dst == NULL)             \
			g_warning(__VA_ARGS__);  \
		g_propagate_error(dst, src); \
	} G_STMT_END

// --
// GObject functions
// --

guint
gel_object_get_ref_count(GObject *object);

const gchar*
gel_object_get_type_name(GObject *object);

// --
// Utilities for GList/GSList
// --

gchar **
gel_list_to_strv(GList *list, gboolean copy);

GList *
gel_strv_to_list(gchar **strv, gboolean copy);

gchar *
gel_list_join(const gchar *separator, GList *list);

gchar **
gel_strv_concat(gchar **strv_a, ...);

/**
 * gel_slist_deep_free:
 * @list: A #GSList
 * @callback: A #GFunc callback
 *
 * See gel_list_deep_free()
 **/
#define gel_slist_deep_free(list,callback) gel_list_deep_free((GList*)list, callback)

/**
 * gel_slist_deep_free_with_data:
 * @list: A #GSList
 * @callback: A #GFunc callback
 * @user_data: User data to pass to @callback as second argument
 *
 * See gel_list_deep_free_with_data()
 **/
#define gel_slist_deep_free_with_data(list,callback,user_data) gel_list_deep_free_with_data((GList*)list,callback,user_data)

/**
 * gel_list_deep_free:
 * @list: A #GList
 * @callback: A #GFunc callback
 *
 * Deep free a #GList. @callback is invoked to free each element of @list
 **/
#define gel_list_deep_free(list,callback) gel_list_deep_free_with_data(list,(GFunc)callback,NULL)

/**
 * gel_list_deep_free_with_data:
 * @list: A #GList
 * @callback: A #GFunc callback
 * @user_data: User data to pass to @callback as second argument
 *
 * Deep free a #GList. @callback is invoked to free each element of @list with
 * @data as second argument
 **/
#define gel_list_deep_free_with_data(list,callback,user_data) \
	do { \
		if (list != NULL) { \
			g_list_foreach(list, callback, user_data); \
			g_list_free(list); \
		} \
	} while (0)

void
gel_list_bisect(GList *input, GList **accepted, GList **rejected, GelFilterFunc callback, gpointer data);

GList*
gel_list_filter(GList *input, GelFilterFunc callback, gpointer user_data);

void
gel_slist_differential_free(GSList *list, GSList *hold, GCompareFunc compare, GFunc callback, gpointer user_data);

void
gel_list_printf(GList *list, const gchar *format, GelPrintFunc stringify_func);

/**
 * gel_slist_printf:
 *
 * See gel_list_printf()
 **/
#define gel_slist_printf(list,format,func) gel_list_printf((GList*) list, format, func)

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

// --
// File system utilities
// --
GList *
gel_dir_read(gchar *path, gboolean absolute, GError **error);

gchar **
gel_file_strings(gchar *pathname);

// --
// XDG related
// --
void
gel_show_to_user(const gchar *uri);

// --
// Timeout functions
// --
guint
gel_run_once_on_idle(GSourceFunc callback, gpointer data, GDestroyNotify destroy);

guint
gel_run_once_on_timeout(guint interval, GSourceFunc callback, gpointer data, GDestroyNotify destroy);

// --
// Date time
// --
gchar *
gel_8601_date_now(void);

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
// strv funcs
// --
gchar **
gel_strv_copy(gchar **strv, gboolean deep);

gchar **
gel_strv_delete(gchar **strv, gint index);

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

/*
 * gel_warn_fix_implementation:
 *
 * Warns about some buggy method
 */
#define gel_warn_fix_implementation() \
	g_warning(_("%s needs to be fixed. Method is incomplete, buggy or does not match documentation."), __FUNCTION__)

void
gel_object_class_print_properties(GObjectClass *object);
void
gel_object_interface_print_properties(gpointer interface);

void
gel_debug_real  (const gchar *domain, GelDebugLevel level, const char *func, const char *file, int line, const char *format, ...);

G_END_DECLS

#endif // __GEL_MISC_H__

