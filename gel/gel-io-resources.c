/*
 * gel/gel-io-resources.c
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

#include "gel-io-resources.h"
#include <glib/gi18n.h>

GFile *
gel_io_file_new(const gchar *path_or_uri)
{
	static GRegex *regex = NULL;
	if (regex == NULL)
		regex = g_regex_new("^[a-z]+://", G_REGEX_CASELESS, 0, NULL);

	if (g_regex_match(regex, path_or_uri, 0, NULL))
		return g_file_new_for_uri(path_or_uri);
	else
		return g_file_new_for_path(path_or_uri);
}

GInputStream *
gel_io_open(const gchar *path_or_uri, GError **error)
{
	GFile *f = gel_io_file_new(path_or_uri);
	GInputStream *ret = G_INPUT_STREAM(g_file_read(f, NULL, error));
	g_object_unref(f);

	return ret;
}

GInputStream*
gel_io_open_or_error(const gchar *path_or_uri)
{
	GError *e = NULL;
	GInputStream *ret = gel_io_open(path_or_uri, &e);
	if (ret == NULL)
		g_error(_("Can't open `%s': %s"), path_or_uri, e->message);
	return ret;
}

gboolean
gel_io_resources_load_file_contents(const gchar *path, gchar **contents, gsize *length, GError **error)
{
  	gchar *tmp = g_strconcat("resource://", path, NULL);
	GFile *f = g_file_new_for_uri(tmp);
	gboolean ret = g_file_load_contents(f, NULL, contents, length, NULL, error);
	g_object_unref(f);

	return ret;
}

