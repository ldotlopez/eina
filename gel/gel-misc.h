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
	GEL_DEBUG_LEVEL_VERBOSE = 5,
	GEL_N_DEBUG_LEVELS      = 6
} GelDebugLevel;

enum {
	GEL_BASENAME,
	GEL_DIRNAME,
	GEL_PATHNAME,
	GEL_N_PATH_STRINGS
};

typedef enum {
	GEL_APP_RESOURCE_UI,    // ui/
	GEL_APP_RESOURCE_IMAGE, // pixmaps/
	GEL_APP_RESOURCE_LIB,   // lib/

	GEL_APP_RESOURCES
} GelAppResourceType;

typedef enum GelFileLoadCode {
	GEL_FILE_LOAD_CODE_OK,
	GEL_FILE_LOAD_CODE_NOT_FOUND,
	GEL_FILE_LOAD_CODE_CANNOT_LOAD,
} GelFileLoadCode;

typedef gboolean (*GelFilterFunc)(const gpointer data, gpointer user_data);

// --
// Initialization and finilization functions
// --
void
gel_init(gchar *app_name, gchar *data_dir);

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

// --
// App resources functions
// --
GList *
gel_app_resource_get_list(GelAppResourceType type, gchar *resource);

gchar *
gel_app_resource_get_pathname(GelAppResourceType type, gchar *resource);

gchar *
gel_app_userdir_get_pathname(gchar *appname, gchar *filename, gboolean create_parents, gint mode);

// --
// File system utilities
// --
GList *
gel_dir_read(gchar *path, gboolean absolute, GError **error);

gchar **
gel_file_strings(gchar *pathname);

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
// Debug functions
// --
GelDebugLevel
gel_get_debug_level(void);
void
gel_set_debug_level(GelDebugLevel level);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define _gel_debug(level,...) gel_debug_real (GEL_DOMAIN, level, __func__, __FILE__, __LINE__, __VA_ARGS__)
#elif defined(__GNUC__) && __GNUC__ >= 3
#define _gel_debug(level,...) gel_debug_real (GEL_DOMAIN, level, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define _gel_debug
#endif

#define gel_verbose(...)    _gel_debug(GEL_DEBUG_LEVEL_VERBOSE, __VA_ARGS__)
#define gel_debug(...)	    _gel_debug(GEL_DEBUG_LEVEL_DEBUG,   __VA_ARGS__)
#define gel_info(...)       _gel_debug(GEL_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#define gel_warn(...)       _gel_debug(GEL_DEBUG_LEVEL_WARN,    __VA_ARGS__)
#define gel_error(...)      _gel_debug(GEL_DEBUG_LEVEL_ERROR,   __VA_ARGS__)
#define gel_apocalipse(...) _gel_debug(GEL_DEBUG_LEVEL_SEVERE,  __VA_ARGS__)
#define gel_implement(...)  gel_warn("IMPLEMENT-ME -- " __VA_ARGS__)
#define gel_fix(...)        gel_warn("FIX-ME -- " __VA_ARGS__)

void
gel_debug_real (const gchar *domain, GelDebugLevel level, const char *func, const char *file, int line, const char *format, ...);

G_END_DECLS

#endif // _GEL_MISC_H

