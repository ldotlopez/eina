/*
 * gel/gel-plugin-info.h
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

#ifndef _GEL_PLUGIN_INFO_H
#define _GEL_PLUGIN_INFO_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	gchar *name;
	gchar *dirname, *pathname;
	gchar *version, *depends, *author, *url;
	gchar *short_desc, *long_desc, *icon_pathname;

	gboolean hidden;
} GelPluginInfo;

enum {
	GEL_PLUGIN_INFO_NO_ERROR = 0,
	GEL_PLUGIN_INFO_NULL_FILENAME,
	GEL_PLUGIN_INFO_CANNOT_OPEN_MODULE,
	GEL_PLUGIN_INFO_SYMBOL_NOT_FOUND,
	GEL_PLUGIN_INFO_HAS_NO_GROUP,
	GEL_PLUGIN_INFO_MISSING_KEY
};

GelPluginInfo *gel_plugin_info_new(const gchar *filename, const gchar *name, GError **error);
void           gel_plugin_info_free(GelPluginInfo *pinfo);
void           gel_plugin_info_copy(const GelPluginInfo *src, GelPluginInfo *dst);
GelPluginInfo *gel_plugin_info_dup(const GelPluginInfo *info);
gboolean       gel_plugin_info_cmp(const GelPluginInfo *a, const GelPluginInfo *b);
#define gel_plugin_info_equal(a,b) (gel_plugin_info_cmp(a,b) == 0)

G_END_DECLS

#endif

