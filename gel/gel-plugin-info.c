/*
 * gel/gel-plugin-info.c
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

#define GEL_DOMAIN "Gel::PluginInfo"

#include "gel-plugin-info.h"

#include <gmodule.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include "gel-misc.h"

GEL_DEFINE_QUARK_FUNC(plugin_info);

GelPluginInfo *
gel_plugin_info_new(const gchar *filename, const gchar *name, GError **error)
{
	if ((filename == NULL) && (name == NULL))
	{
		g_set_error(error, plugin_info_quark(), GEL_PLUGIN_INFO_NULL_FILENAME, N_("A name must be provided if filename is NULL"));
		g_return_val_if_fail((filename != NULL) && (name != NULL), NULL);
	}

	GelPluginInfo *info = NULL;

	// Load as internal
	if (filename == NULL)
	{
		GModule *m = g_module_open(NULL, G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL);
		if (!m)
		{
			g_set_error(error, plugin_info_quark(), GEL_PLUGIN_INFO_CANNOT_OPEN_MODULE, N_("Unable to introspect NULL"));
			return NULL;
		}

		gchar *symbol = g_strconcat(name, "_plugin_info", NULL);
		GelPluginInfo *symbol_p = NULL;
		if (!g_module_symbol(m, symbol, (gpointer *) &symbol_p))
		{
			g_set_error(error, plugin_info_quark(), GEL_PLUGIN_INFO_SYMBOL_NOT_FOUND, N_("Symbol '%s' not found"), symbol);
			g_free(symbol);
			g_module_close(m);
			return NULL;
		}
		info = gel_plugin_info_dup(symbol_p);
		g_module_close(m);
	}

	// Load as external
	else
	{
		GKeyFile *kf = g_key_file_new();
		if (!g_key_file_load_from_file(kf, filename, 0, error))
		{
			g_key_file_free(kf);
			return NULL;
		}

		if (!g_key_file_has_group(kf, "Eina Plugin"))
		{	
			g_set_error(error, plugin_info_quark(), GEL_PLUGIN_INFO_HAS_NO_GROUP, N_("Info file has no group named 'Eina Plugin'"));
			g_key_file_free(kf);
			return NULL;
		}

		gchar *required_keys[] = {"name", "version", "url", "author", "depends"};
		for (gint i = 0; i < G_N_ELEMENTS(required_keys); i++)
		{
			if (!g_key_file_has_key(kf, "Eina Plugin", required_keys[i], NULL))
			{
				g_set_error(error, plugin_info_quark(), GEL_PLUGIN_INFO_MISSING_KEY, N_("Missing required key '%s'"), required_keys[i]);
				g_key_file_free(kf);
				return NULL;
			}
		}

		info = g_new0(GelPluginInfo, 1);

		info->pathname = g_strdup(filename);
		info->dirname  = g_path_get_dirname(filename);
		info->name     = g_key_file_get_string(kf, "Eina Plugin", "name",    NULL);

		info->version  = g_key_file_get_string(kf, "Eina Plugin", "version", NULL);
		info->depends  = g_key_file_get_string(kf, "Eina Plugin", "depends", NULL);
		info->author   = g_key_file_get_string(kf, "Eina Plugin", "author",  NULL);
		info->url      = g_key_file_get_string(kf, "Eina Plugin", "url",     NULL);

		info->short_desc = g_key_file_get_locale_string(kf, "Eina Plugin", "short-description", NULL, NULL);
		info->long_desc  = g_key_file_get_locale_string(kf, "Eina Plugin", "long-description",  NULL, NULL);
		info->icon_pathname = g_key_file_get_string(kf, "Eina Plugin", "icon", NULL);

		gchar *hidden_str = g_key_file_get_string(kf, "Eina Plugin", "hidden", NULL);
		if (!hidden_str || g_str_equal(hidden_str,"false") || g_str_equal(hidden_str, "FALSE") || g_str_equal(hidden_str, "0"))
			info->hidden = FALSE;
		else
			info->hidden = TRUE;
		g_free(hidden_str);
	}

	if (info->icon_pathname && !g_path_is_absolute(info->icon_pathname))
	{
		gchar *old = info->icon_pathname;
		if (info->dirname)
			info->icon_pathname = g_build_filename(info->dirname, old, NULL);
		else
			info->icon_pathname = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, old);
		g_free(old);
	}

	return info;
}

void
gel_plugin_info_free(GelPluginInfo *info)
{
	gel_free_and_invalidate(info->name,     NULL, g_free);
	gel_free_and_invalidate(info->dirname,  NULL, g_free);
	gel_free_and_invalidate(info->pathname, NULL, g_free);

	gel_free_and_invalidate(info->version, NULL, g_free);
	gel_free_and_invalidate(info->depends, NULL, g_free);
	gel_free_and_invalidate(info->author,  NULL, g_free);
	gel_free_and_invalidate(info->url ,    NULL, g_free);

	gel_free_and_invalidate(info->short_desc, NULL, g_free);
	gel_free_and_invalidate(info->long_desc,  NULL, g_free);
	gel_free_and_invalidate(info->icon_pathname, NULL, g_free);
}

gboolean
gel_plugin_info_cmp(const GelPluginInfo *a, const GelPluginInfo *b)
{
	if (!a->pathname && b->pathname)
		return 1;
	if (a->pathname && !b->pathname)
		return -1;
	return strcmp(a->name, b->name);
}

void
gel_plugin_info_copy(const GelPluginInfo *src, GelPluginInfo *dst)
{
	dst->name = src->name ? g_strdup(src->name) : NULL;
	dst->dirname  = src->dirname  ? g_strdup(src->dirname) : NULL;
	dst->pathname = src->pathname ? g_strdup(src->pathname) : NULL;

	dst->version = src->version ? g_strdup(src->version) : NULL;
	dst->depends = src->depends ? g_strdup(src->depends) : NULL;
	dst->author  = src->author  ? g_strdup(src->author)  : NULL;
	dst->url     = src->url     ? g_strdup(src->url)     : NULL;

	dst->short_desc = src->short_desc ? g_strdup(src->short_desc) : NULL;
	dst->long_desc  = src->long_desc  ? g_strdup(src->long_desc)  : NULL;
	dst->icon_pathname = src->icon_pathname ? g_strdup(src->icon_pathname) : NULL;

	dst->hidden = src->hidden;
}

GelPluginInfo *
gel_plugin_info_dup(const GelPluginInfo *info)
{
	GelPluginInfo *ret = g_new0(GelPluginInfo, 1);
	gel_plugin_info_copy(info, ret);
	return ret;
}

