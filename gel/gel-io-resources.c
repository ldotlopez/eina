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

gboolean
gel_io_resources_load_file_contents(const gchar *path, gchar **contents, gsize *length, GError **error)
{
  	gchar *tmp = g_strconcat("resource://", path, NULL);
	GFile *f = g_file_new_for_uri(tmp);
	gboolean ret = g_file_load_contents(f, NULL, contents, length, NULL, error);
	g_object_unref(f);

	return ret;
}

