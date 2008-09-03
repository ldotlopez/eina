#include <stdlib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include "gel.h"

static gchar *_gel_package_name = NULL;
static gchar *_gel_package_data_dir = NULL;
static gint   _gel_debug_level = GEL_DEBUG_LEVEL_INFO;

void _gel_atexit(void);

void
gel_init(gchar *name, gchar *data_dir)
{
	_gel_package_name     = g_strdup(name);
	_gel_package_data_dir = g_strdup(data_dir);
	atexit(_gel_atexit);
}

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
_gel_atexit(void)
{
	if (_gel_package_name != NULL)
		g_free(_gel_package_name);
	if (_gel_package_data_dir!= NULL)
		g_free(_gel_package_data_dir);
}

void
gel_debug_real(const gchar *domain, GelDebugLevel level, const char *func, const char *file, int line, const char *format, ...)
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

	va_list args;
	char buffer[1025];

	if (level > _gel_debug_level)
		return;

	va_start (args, format);
	g_vsnprintf (buffer, 1024, format, args);
	va_end (args);

	g_printf("[%s] [%s (%s:%d)] %s\n", level_strs[level], domain , file, line, buffer);
}

/*
 * List <-> strv functions
 */
gchar **
gel_glist_to_strv(GList *list, gboolean copy)
{
	GList *iter;
	guint len;
	gchar **ret;
	gint i;

	if (list == NULL)
		return NULL;

	len = g_list_length(list);
	if (len <= 0)
		return NULL;

	ret = g_new0(gchar*, len + 1);

	for (iter = list, i = 0; iter != NULL; iter = iter->next, i++)
	{
		if (copy) 
			ret[i] = g_strdup((gchar *) iter->data);
		else 
			ret[i] = (gchar *) iter->data;
	}
	return ret;
}

GList *
gel_strv_to_glist(gchar **strv, gboolean copy)
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

/*
 * String utility functions
 */
gchar *
gel_glist_join(const gchar *separator, GList *list)
{
	gchar *ret;
	gchar **tmp = gel_glist_to_strv(list, FALSE);

	if (tmp == NULL)
		return NULL;
	ret = g_strjoinv(separator, tmp);
	g_free(tmp);

	return ret;
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

#ifndef GEL_DISABLE_DEPRECATED
void
gel_file_strings_free(gchar **file_strings)
{
	g_strfreev(file_strings);
}
#endif


/*
 * App resource functions
 */
GList *
gel_app_resource_get_list(GelAppResourceType type, gchar *resource)
{
	GList  *ret = NULL;
	gchar  *tmp;
	gchar  *envvar;
	gchar  *env_uppercase;
	gchar  *package_uppercase;
	gchar  *dot_packagename;
	gchar  *map_table[GEL_APP_RESOURCES] = { "ui", "pixmaps", "lib" };

    /* Create a list with some posible paths */
	/*
	 * 1. Add path from env variable
	 * 2. Add from user's dir (~/.PACKAGE/filetype/filename)
	 * 3. Use system dir (_g_ext_package_data_dir/filetype/filename)
	 */

	/* Create the filename from envvar:
	 * uppercase the filetype -> get the envvar -> build filename
	 * add fullpath to the list
	 * free temporal data
	 */
	env_uppercase      = g_ascii_strup(map_table[type], -1);
	package_uppercase  = g_ascii_strup(_gel_package_name, -1);
	envvar    = g_strconcat(package_uppercase, "_", env_uppercase, "_DIR", NULL);
	if (g_getenv(envvar) != NULL ) {
		ret = g_list_append(ret,  g_build_filename(g_getenv(envvar), resource, NULL));
	}
	g_free(envvar);
	g_free(env_uppercase);
	g_free(package_uppercase);

	/* Create the filename from user's dir */
	dot_packagename = g_strconcat(".", _gel_package_name, NULL);
	tmp = g_build_filename(g_get_home_dir(), dot_packagename, map_table[type], resource, NULL);
	g_free(dot_packagename);
	ret = g_list_append(ret, tmp);

	/* Build the system dir */
	tmp = g_build_filename(_gel_package_data_dir, _gel_package_name, map_table[type], resource, NULL);
	ret = g_list_append(ret, tmp);
	return ret;
}

gchar *
gel_app_resource_get_pathname(GelAppResourceType type, gchar *resource)
{
	GList *candidates, *iter;
	gchar *ret = NULL;

	iter = candidates = gel_app_resource_get_list(type, resource);

	while (iter)
	{
		/* XXX: Follow symlinks?? */
		if(g_file_test((gchar *) iter->data, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
			ret = (gchar *) iter->data;
			break;
		}
		iter = iter->next;
	}

	/* Free the list and elements except for found element */
	iter = candidates ;
	while (iter)
	{
		if (iter->data != ret)
			g_free(iter->data);
		iter = iter->next;
	}
	g_list_free(candidates);

	return ret;
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
