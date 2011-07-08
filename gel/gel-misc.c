/*
 * gel/gel-misc.c
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

#include "gel-misc.h"

#include <stdlib.h>
#include <errno.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include "gel.h"

extern int errno;

// --
// Internal variables
// --
static gchar *_gel_package_name = NULL;
static gchar *_gel_package_data_dir = NULL;
static gchar *_gel_package_lib_dir  = NULL;

/**
 * SECTION:gel-initialization
 *
 * Initilization of libgel
 **/

static void
_gel_atexit(void)
{
	gel_free_and_invalidate(_gel_package_name,        NULL, g_free);
	gel_free_and_invalidate(_gel_package_data_dir,    NULL, g_free);
}

/**
 * gel_init:
 *
 * @package_name: Name of the package/app using libgel. Something like 'foo' or 'bar'.
 * @lib_dir: libdir for the app, in most cases something like /usr/local/lib or
 *           /usr/lib
 * @data_dir: datadir for the app, in most cases something like
 *            /usr/local/share or /usr/share
 *
 * Initializes the library
 **/
void
gel_init(const gchar *package_name, const gchar *lib_dir, const gchar *data_dir)
{
	g_set_prgname(package_name);
	_gel_package_name     = g_strdup(package_name);
	_gel_package_lib_dir  = g_strdup(lib_dir);
	_gel_package_data_dir = g_strdup(data_dir);
	atexit(_gel_atexit);
}

/**
 * gel_get_package_name:
 *
 * Get package name from libgel
 *
 * Returns: (transfer none): package name of the app using libgel.
 *          This string is owned by libgel and should NOT be freeded.
 **/
const gchar*
gel_get_package_name(void)
{
	return (const gchar *) _gel_package_name;
}

/**
 * gel_get_package_lib_dir:
 *
 * Get package libdir from libgel
 *
 * Returns: (transfer none): package libdir of the app using libgel
 *          This string is owned by libgel and should NOT be freeded.
 **/
const gchar*
gel_get_package_lib_dir(void)
{
	return (const gchar *) _gel_package_lib_dir;
}

/**
 * gel_get_package_data_dir:
 *
 * Get package data_dir from libgel
 *
 * Returns: (transfer none): package data_dir of the app using libgel
 *          This string is owned by libgel and should NOT be freeded.
 **/
const gchar*
gel_get_package_data_dir(void)
{
	return (const gchar *) _gel_package_data_dir;
}

/**
 * SECTION:gel-object
 * @short_description: Functions handling or embracing #GObject's
 * @see_also: #GObject
 *
 * libgel support to #GObject
 */

/**
 * gel_object_get_ref_count:
 *
 * @object: A #GObject
 *
 * Get the number of references over the object
 *
 * Returns: The number of references
 **/
guint
gel_object_get_ref_count(GObject *object)
{
	return G_OBJECT(object)->ref_count;
}

/**
 * gel_object_get_type_name:
 *
 * @object: A #GObject
 *
 * Get object's class printable name
 *
 * Returns: (transfer none): A string containing the object's class name. This string is owned
 *          by glib-object and should NOT be freeded.
 **/
const gchar*
gel_object_get_type_name(GObject *object)
{
	return G_OBJECT_TYPE_NAME(object);
}

/**
 * SECTION:gel-list
 * @short_description: GList/GSList support function
 * @see_also: #GList, #GSList
 *
 * libgel support for lists
 */

/**
 * gel_list_to_strv:
 *
 * @list: A #GList to convert to gchar**
 * @copy: if %TRUE the data is also copied
 *
 * Converts a #GList to a gchar** optionally copying data (a gchar element-type is
 * assumed)
 *
 * Returns: A gchar** with the elements of @list
 **/
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

/**
 * gel_strv_to_list:
 *
 * @strv: A %NULL-terminated array of strings
 * @copy: if %TRUE the data is also copied
 *
 * Coverts a gchar** to a #GList, see also gel_list_to_strv()
 *
 * Returns: A #GList with the elements of @strv
 **/
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

/**
 * gel_strlist_join:
 *
 * @separator: Separator for use
 * @list: A #GList to join
 *
 * Joins a #GList into a single string using @separator as union, see also
 * #g_strjoinv
 *
 * Returns: A newly allocated string which must be freeded when it is not
 *          necessary.
 */
gchar *
gel_strlist_join(const gchar *separator, GList *list)
{
	gchar *ret;
	gchar **tmp = gel_list_to_strv(list, FALSE);

	if (tmp == NULL)
		return NULL;
	ret = g_strjoinv(separator, tmp);
	g_free(tmp);

	return ret;
}

/**
 * gel_strv_concat:
 *
 * @strv_a: The first %NULL-terminate array of strings
 * @Varargs: a %NULL-terminated list of %NULL-terminate array of strings
 *
 * Concatenates a set of %NULL-terminate array of strings (commonly refered as
 * strv) into a single one.
 *
 * Returns: (transfer full): the new strv with all the strings copied
 */
gchar**
gel_strv_concat(gchar **strv_a, ...)
{
	g_return_val_if_fail(strv_a != NULL, NULL);

	gchar **ret = NULL;
	va_list strvs;
	va_start(strvs, strv_a);

	guint i = 0;

	gchar **strv = strv_a;
	do
	{
		guint l = g_strv_length(strv);

		ret = g_realloc_n(ret, i + l + 1, sizeof(gchar*));
		for (guint j = 0; j < l; j++)
		{
			ret[i++] = g_strdup(strv[j]);
		}
		ret[i] = NULL;

		strv = va_arg(strvs, gchar**);
	} while (strv && strv[0]);

	va_end(strvs);

	return ret;
}

/**
 * gel_list_bisect:
 *
 * @input: A #GList to bisect
 * @accepted: (inout): A #GList to store accepted elements, %NULL is not
 *                     allowed
 * @rejected: (inout): A #GList to store rejected elements, %NULL is not
 *                     allowed
 * @callback: A #GelFilterFunc
 * @data: data to pass to @callback
 *
 * Bisect (split) a #GList based on the criteria of @callback. Callback is
 * called in form of callback(element,data). If @callback returns %TRUE
 * element is stored in accepted, else element is stored in rejected.
 *
 * Original list is completly freeded, since all of his elements are moved to
 * @accepted or @rejected. Neither of @accepted or @rejected can be %NULL
 * because elements will be leaked.
 **/
void
gel_list_bisect(GList *input, GList **accepted, GList **rejected, GelFilterFunc callback, gpointer data)
{
	GList *iter = input;
	while (iter)
	{
		input = g_list_remove_link(input, iter);
		if (callback(iter->data, data))
			*accepted = g_list_concat(*accepted, iter);
		else
			*rejected = g_list_concat(*rejected, iter);
		iter = input;
	}
}

/**
 * gel_list_filter:
 *
 * @input: A #GList to filter
 * @callback: A #GelFilterFunc used for filtering
 * @user_data: data to pass to @callback
 *
 * Filters @input's elements to return a newly created #GList based on the
 * criteria of @callback, if %TRUE is returned, element is included in the
 * returned list.
 *
 * Note that elements itself are not copied.
 *
 * See also: gel_list_bisect()
 *
 * Returns: (transfer container): The filtered list
 **/
GList*
gel_list_filter(GList *input, GelFilterFunc callback, gpointer user_data)
{
	GList *ret = NULL;
	while (input)
	{
		if (callback(input->data, user_data))
			ret = g_list_prepend(ret, input->data);
		input = input->next;
	}
	return g_list_reverse(ret);
}

/**
 * gel_list_printf:
 * 
 * @list: A #GList
 * @format: (allow-none): Format to use for print each the stringified element. By default
 *                        "%s\n"
 * @stringify_func: The #GelPrintFunc to use for stringify elements
 *
 * Prints a #GList to stdout using @format for each element and @stringify_func
 * to stringify elements.
 **/
void
gel_list_printf(GList *list, const gchar *format, GelPrintFunc stringify_func)
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
	const gchar *map_table[GEL_RESOURCE_N_TYPES] = { "UI", "PIXMAP", "LIB", NULL };
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
	const gchar *map_table[GEL_RESOURCE_N_TYPES] = { "ui", "pixmaps", "lib" , "." };

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
	gchar *data_key = g_strconcat(prgupper, "_DATA_PREFIX", NULL);
	gchar *lib_key  = g_strconcat(prgupper, "_LIB_PREFIX",  NULL);

	gchar *data_val = g_strdup_safe(g_getenv(data_key));
	gchar *lib_val  = g_strdup_safe(g_getenv(lib_key));

	g_free(data_key);
	g_free(lib_key);

	if (!data_val)
		data_val = g_strdup(gel_get_package_data_dir());
	if (!lib_val)
		lib_val  = g_strdup(gel_get_package_lib_dir());

	gchar *ret = NULL;
	switch (type)
	{
	case GEL_RESOURCE_TYPE_IMAGE:
		if (data_val)
			ret = g_build_filename(data_val, "pixmaps", NULL);
		break;
		
	case GEL_RESOURCE_TYPE_UI:
		if (data_val)
			ret = g_build_filename(data_val, "ui", NULL);
		break;

	case GEL_RESOURCE_TYPE_SHARED:
		ret = g_strdup(lib_val);
		break;

	case GEL_RESOURCE_TYPE_OTHER:
		break;

	default:
		g_warning(_("Unknow GelResourceType %d"), type);
		break;
	}

	g_free(data_val);
	g_free(lib_val);

	return ret;
}

GList *
gel_resource_locate_list(GelResourceType type, gchar *resource)
{
	// Search precedence
	// 1. Enviroment variables
	// If enviroment variable is NULL:
	//   2.1. User's dir based on GelPluginResourceType
	//   2.2. System or plugin dir based on GelPluginResourceType
	const gchar *searchpath = gel_resource_type_get_env(type);
	if (searchpath)
	{
		gchar **searchpaths = g_strsplit(searchpath, G_SEARCHPATH_SEPARATOR_S, -1);
	
		GList *ret = NULL;
		gint i = 0;
		while (searchpaths[i] && searchpaths[i][0])
		{
			ret = g_list_append(ret, g_strconcat(searchpaths[i], G_DIR_SEPARATOR_S, resource, NULL));
			i++;
		}
		g_strfreev(searchpaths);
		return ret;
	}

	GList *ret = NULL;
	gchar *userdir = gel_resource_type_get_user_dir(type);
	if (userdir)
	{
		ret = g_list_prepend(ret, g_build_filename(userdir, resource, NULL));
		g_free(userdir);
	}

	gchar *systemdir = gel_resource_type_get_system_dir(type);
	if (systemdir)
	{
		ret = g_list_prepend(ret, g_build_filename(systemdir, resource, NULL));
		g_free(systemdir);
	}

	return g_list_reverse(ret);
}

gchar *
gel_resource_locate(GelResourceType type, gchar *resource)
{
	GList *candidates = gel_resource_locate_list(type, resource);
	GList *iter = candidates;
	gchar *ret = NULL;
	while (iter)
	{
		if (g_file_test((gchar *) iter->data, G_FILE_TEST_IS_REGULAR))
		{
			ret = g_strdup((gchar *) iter->data);
			break;
		}
		iter = iter->next;
	}
	gel_list_deep_free(candidates, g_free);
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
	gchar **ret = g_new0(gchar*, GEL_PATH_N_COMPONENTS);
	ret[GEL_PATH_COMPONENT_PATHNAME] = g_strdup(pathname);
	ret[GEL_PATH_COMPONENT_BASENAME] = g_path_get_basename(pathname);
	ret[GEL_PATH_COMPONENT_DIRNAME]  = g_path_get_dirname(pathname);

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
// Date time
// --
gchar *
gel_8601_date_now(void)
{
	GTimeVal now; g_get_current_time(&now);
	return g_time_val_to_iso8601(&now);
}

// --
// strv funcs
// --
gchar **
gel_strv_copy(gchar **strv, gboolean deep)
{
	gchar **ret = g_new0(gchar *, g_strv_length(strv) + 1);
	for (guint i = 0; strv[i] != NULL; i++)
		if (deep)
			ret[i] = g_strdup(strv[i]);
		else
			ret[i] = strv[i];
	return ret;
}

gchar **
gel_strv_delete(gchar **str, gint index)
{
	g_return_val_if_fail(str != NULL, NULL);
	g_return_val_if_fail((index > 0) && (index < g_strv_length(str)), NULL);

	guint  i = index;
	gchar *p = str[i];
	for (i = index; str[i]; i++)
		str[i] = str[i+1];
	g_free(p);

	return g_realloc(str, sizeof(gchar*) * i);
}

/**
 * SECTION: GObject helper functions
 */
void
gel_object_class_print_properties(GObjectClass *klass)
{
	guint n_specs;
	GParamSpec **specs = g_object_class_list_properties(klass, &n_specs);
	const gchar *class_name = G_OBJECT_CLASS_NAME(klass); 
	for (guint i = 0; i < n_specs; i++)
	{
		g_debug("%s:%s %s", class_name,
			g_param_spec_get_name(specs[i]),
			g_type_name(specs[i]->value_type)
			);
	} 
}

void
gel_object_interface_print_properties(gpointer iface)
{
	guint n_specs;
	GParamSpec **specs = g_object_interface_list_properties(iface, &n_specs);
	const gchar *class_name = G_OBJECT_CLASS_NAME(iface);
	for (guint i = 0; i < n_specs; i++)
	{
		g_debug("%s:%s %s", class_name,
			g_param_spec_get_name(specs[i]),
			g_type_name(specs[i]->value_type)
			);
	}
}
