/*
 * gel/gel-misc.c
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

#include <stdlib.h>
#include <errno.h>
#include <glib/gprintf.h>
#include <gel/gel.h>

extern int errno;

// --
// Internal variables
// --
static gchar *_gel_package_name = NULL;
static gchar *_gel_package_data_dir = NULL;
static gchar *_gel_package_lib_dir  = NULL;
static gint   _gel_debug_level = GEL_DEBUG_LEVEL_INFO;

static GList *_gel_debug_handlers      = NULL;
static GList *_gel_debug_handlers_data = NULL;

void
gel_debug_default_handler(GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer);

// --
// Initialization and finilization functions
// --
void
_gel_atexit(void)
{
	gel_free_and_invalidate(_gel_package_name,        NULL, g_free);
	gel_free_and_invalidate(_gel_package_data_dir,    NULL, g_free);
	gel_free_and_invalidate(_gel_debug_handlers,      NULL, g_list_free);
	gel_free_and_invalidate(_gel_debug_handlers_data, NULL, g_list_free);
}

void
gel_init(gchar *name, gchar *lib_dir, gchar *data_dir)
{
	_gel_package_name     = g_strdup(name);
	_gel_package_lib_dir  = g_strdup(lib_dir);
	_gel_package_data_dir = g_strdup(data_dir);
	_gel_debug_handlers      = g_list_append(NULL, gel_debug_default_handler);
	_gel_debug_handlers_data = g_list_append(NULL, NULL);
	atexit(_gel_atexit);
}

const gchar*
gel_get_package_name(void)
{
	return (const gchar *) _gel_package_name;
}

const gchar*
gel_get_package_lib_dir(void)
{
	return (const gchar *) _gel_package_lib_dir;
}

const gchar*
gel_get_package_data_dir(void)
{
	return (const gchar *) _gel_package_data_dir;
}

// --
// Utilities for GList/GSList
// --
//
gchar **
gel_list_to_strv(GList *list, gboolean copy)
{
	GList *iter;
	guint len;
	gchar **ret;
	gint i;

	g_return_val_if_fail(list != NULL, NULL);

	len = g_list_length(list);
	g_return_val_if_fail(len > 0, NULL);

	ret = g_new0(gchar*, len + 1);
	for (iter = list, i = 0; iter != NULL; iter = iter->next, i++)
		ret[i] = copy ? g_strdup((gchar *) iter->data) : iter->data;

	return ret;
}

GList *
gel_strv_to_list(gchar **strv, gboolean copy)
{
	GList *ret = NULL;
	gint i = 0;

	while (strv[i] != NULL)
	{
		if (copy)
			ret = g_list_prepend(ret, g_strdup(strv[i]));
		else
			ret = g_list_prepend(ret, strv[i]);
		i++;
	}

	return g_list_reverse(ret);
}

gchar *
gel_list_join(const gchar *separator, GList *list)
{
	gchar *ret;
	gchar **tmp = gel_list_to_strv(list, FALSE);

	if (tmp == NULL)
		return NULL;
	ret = g_strjoinv(separator, tmp);
	g_free(tmp);

	return ret;
}

GSList*
gel_slist_filter(GSList *input, GelFilterFunc callback, gpointer user_data)
{
	GSList *ret = NULL;
	while (input)
	{
		if (callback(input->data, user_data))
			ret = g_slist_prepend(ret, input->data);
		input = input->next;
	}
	return g_slist_reverse(ret);
}

void
gel_slist_differential_free(GSList *list, GSList *hold, GCompareFunc compare, GFunc callback, gpointer user_data)
{
	while (list)
	{
		if (!compare(list->data, hold->data))
		{
			callback(list->data, user_data);
			hold = hold->next;
		}
		list = list->next;
	}
}

void
gel_list_printf(GList *list, gchar *format, GelListPrintfFunc stringify_func)
{
	g_printf("Contents of %p\n", list);
	while (list)
	{
		gchar *str = stringify_func((const gpointer) list->data);
		g_printf("[%p] ", list->data);
		g_printf(format ? format : "%s\n", str ? str : "(NULL)");
		if (str != NULL)
			g_free(str);
		list = list->next;
	}
	g_printf("End of list\n");
}

// --
// App resources functions
// --
const gchar *
gel_resource_type_get_env(GelResourceType type)
{
	const gchar *map_table[GEL_N_RESOURCES] = { "UI", "PIXMAP", "LIB", NULL };
	if (!map_table[type])
		return NULL;

	gchar *upper_prgname = g_ascii_strup((const gchar*) g_get_prgname(), -1);

	gchar *env_name = g_strconcat(upper_prgname, "_", map_table[type], "_", "PATH", NULL);
	g_free(upper_prgname);
	const gchar *ret = g_getenv(env_name);
	g_free(env_name);

	return ret;
}

gchar *
gel_resource_type_get_user_dir(GelResourceType type)
{
	const gchar *map_table[GEL_N_RESOURCES] = { "ui", "pixmaps", "lib" , "." };

	gchar *ret;
	const gchar *datadir = g_get_user_data_dir();
	if (datadir)
		ret = g_strconcat(datadir, G_DIR_SEPARATOR_S, g_get_prgname(), G_DIR_SEPARATOR_S, map_table[type], NULL);
	else
		ret = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".", g_get_prgname(), G_DIR_SEPARATOR_S, map_table[type], NULL);

	return ret;
}

#define g_strdup_safe(x) (x ? g_strdup(x) : NULL)
gchar *
gel_resource_type_get_system_dir(GelResourceType type)
{
	gchar *prgupper = g_ascii_strup(g_get_prgname(), -1);
	gchar *path_key = g_strconcat(prgupper, "_PATH", NULL);
	gchar *data_key = g_strconcat(prgupper, "_DATA_PREFIX", NULL);
	gchar *lib_key  = g_strconcat(prgupper, "_LIB_PREFIX",  NULL);

	gchar *path_val = g_strdup_safe(g_getenv(path_key));
	gchar *data_val = g_strdup_safe(g_getenv(data_key));
	gchar *lib_val  = g_strdup_safe(g_getenv(lib_key));

	g_free(path_key);
	g_free(data_key);
	g_free(lib_key);

	if (!path_val)
		path_val = g_strdup(PACKAGE_PREFIX);

	if (path_val && !data_val)
		data_val = g_build_filename(path_val, "share", NULL);
	if (path_val && !lib_val)
		lib_val  = g_build_filename(path_val, "lib", NULL);

	gchar *ret = NULL;
	switch (type)
	{
	case GEL_RESOURCE_IMAGE:
		if (data_val)
			ret = g_build_filename(data_val, g_get_prgname(), "pixmaps", NULL);
		break;
		
	case GEL_RESOURCE_UI:
		if (data_val)
			ret = g_build_filename(data_val, g_get_prgname(), "ui", NULL);
		break;

	case GEL_RESOURCE_SHARED:
		if (lib_val)
			ret = g_build_filename(lib_val, g_get_prgname(), NULL);
		break;

	case GEL_RESOURCE_OTHER:
		break;

	default:
		break;
	}

	g_free(path_val);
	g_free(data_val);
	g_free(lib_val);
	return ret;
}

gchar *
gel_resource_locate(GelResourceType type, gchar *resource)
{
	return gel_plugin_get_resource((GelPlugin *) 0x1, type, resource);
}

gchar *
gel_app_userdir_get_pathname(gchar *appname, gchar *filename, gboolean create_parents, gint mode)
{
	gchar *ret, *dirname;

	ret = g_strconcat(
		g_get_home_dir(),
		G_DIR_SEPARATOR_S, ".", appname,
		G_DIR_SEPARATOR_S, filename,
		NULL);

	if (create_parents)
	{
		dirname = g_path_get_dirname(ret);
		g_mkdir_with_parents(dirname, mode);
		g_free(dirname);
	}

	return ret;
}

gchar *
gel_app_build_config_filename(gchar *name, gboolean create_path, gint dir_mode, GError **error)
{
	const gchar *conf_dir = g_get_user_config_dir();
	if (conf_dir == NULL)
		conf_dir = ".config";

	gchar *ret = g_build_filename(conf_dir, _gel_package_name, name, NULL);
	gchar *dirname = g_path_get_dirname(ret);
	if (!g_file_test(dirname, G_FILE_TEST_EXISTS))
	{
		if (g_mkdir_with_parents(dirname, dir_mode) == -1)
		{
			g_set_error_literal(error, G_FILE_ERROR, g_file_error_from_errno(errno), g_strerror(errno));
			g_free(dirname);
			g_free(ret);
		return NULL;
		}
	}
	g_free(dirname);
	return ret;
}

// --
// File system utilities
// --
GList *
gel_dir_read(gchar *path, gboolean absolute, GError **error)
{
	GList *ret = NULL;
	GDir *d;
	const gchar *entry;

	d = g_dir_open(path, 0, error);
	if (d == NULL)
		return NULL;
	
	while ((entry = g_dir_read_name(d)) != NULL)
	{
		if (absolute)
			ret = g_list_prepend(ret, g_build_filename(path, entry, NULL));
		else
			ret = g_list_prepend(ret, g_strdup(entry));
	}
	return g_list_reverse(ret);
}

gchar **
gel_file_strings(gchar *pathname)
{
	gchar **ret = g_new0(gchar*, GEL_N_PATH_STRINGS);
	ret[GEL_PATHNAME] = g_strdup(pathname);
	ret[GEL_BASENAME] = g_path_get_basename(ret[GEL_PATHNAME]);
	ret[GEL_DIRNAME]  = g_path_get_dirname(ret[GEL_PATHNAME]);

	return ret;
}

// --
// Timeout functions
// --
typedef struct {
	GSourceFunc    callback;
	gpointer       data;
	GDestroyNotify destroy;
} GelRunOnce;

static gboolean
gel_run_once_callback_helper(gpointer data)
{
	GelRunOnce *ro = (GelRunOnce *)  data;
	ro->callback(ro->data);
	return FALSE;
}

static void 
gel_run_once_destroy_helper(gpointer data)
{
	GelRunOnce *ro = (GelRunOnce *) data;
	if (ro->destroy)
		ro->destroy(ro->data);
	g_free(ro);
}

guint
gel_run_once_on_idle(GSourceFunc callback, gpointer data, GDestroyNotify destroy)
{
	GelRunOnce *ro = g_new0(GelRunOnce, 1);
	ro->callback = callback;
	ro->data     = data;
	ro->destroy  = destroy;
	return g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, gel_run_once_callback_helper, ro, gel_run_once_destroy_helper);
}

guint
gel_run_once_on_timeout(guint interval, GSourceFunc callback, gpointer data, GDestroyNotify destroy)
{
	GelRunOnce *ro = g_new0(GelRunOnce, 1);
	ro->callback = callback;
	ro->data     = data;
	ro->destroy  = destroy;

	return g_timeout_add_full(interval, G_PRIORITY_DEFAULT_IDLE, gel_run_once_callback_helper, ro, gel_run_once_destroy_helper);
}


// --
// Debug functions
// --
GelDebugLevel
gel_get_debug_level(void)
{
	return _gel_debug_level;
}

void
gel_set_debug_level(GelDebugLevel level)
{
	_gel_debug_level = level;
}

void
gel_debug_add_handler(GelDebugHandler func, gpointer data)
{
	if ((_gel_debug_handlers != NULL) && (_gel_debug_handlers->next == NULL))
	{
		g_list_free(_gel_debug_handlers);
		g_list_free(_gel_debug_handlers_data);
		_gel_debug_handlers = NULL;
		_gel_debug_handlers_data = NULL;
	}

	_gel_debug_handlers = g_list_append(_gel_debug_handlers, (gpointer) func);
	_gel_debug_handlers_data = g_list_append(_gel_debug_handlers_data, (gpointer) data);
}

void
gel_debug_remove_handler(GelDebugHandler func)
{
	gint index = g_list_index(_gel_debug_handlers, func);
	if (index >= 0)
	{
		_gel_debug_handlers = g_list_remove_all(_gel_debug_handlers, (gpointer) func);
		_gel_debug_handlers_data = g_list_remove_all(_gel_debug_handlers_data, g_list_nth_data(_gel_debug_handlers_data, index));
	}

	if (_gel_debug_handlers == NULL)
	{
		_gel_debug_handlers = g_list_append(NULL, gel_debug_default_handler);
		_gel_debug_handlers_data = g_list_append(NULL, _gel_debug_handlers_data);
	}
}

void
gel_debug_default_handler(GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer)
{
	static const gchar *level_strs[] = {
		"\033[1;41mSEVERE\033[m",
		"\033[1;31mERROR\033[m",
		"\033[1;33mWARN\033[m",
		"\033[1mINFO\033[m",
		"\033[1mDEBUG\033[m",
		"\033[1mVERBOSE\033[m",
		NULL
	};

	g_printf("[%s] [%s (%s:%d)] %s\n", level_strs[level], domain , file, line, buffer);
}

void
gel_debug_real(const gchar *domain, GelDebugLevel level, const char *func, const char *file, int line, const char *format, ...)
{
	va_list args;

	if (level > _gel_debug_level)
		return;

	va_start(args, format);
	gchar *buffer = g_strdup_vprintf(format, args);
	va_end(args);

	GList *iter = _gel_debug_handlers;
	GList *data = _gel_debug_handlers_data;
	while (iter)
	{
		GelDebugHandler callback = (GelDebugHandler) iter->data;
		callback(level, domain, func, file, line, buffer, data ? data->data : NULL);
		iter = iter->next;

		if (data)
			data = data->next;
	}

	g_free(buffer);
}
