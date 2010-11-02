/*
 * eina/ext/eina-file-utils.c
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

#include "eina-file-utils.h"

gboolean
eina_file_utils_is_supported_extension(gchar *uri)
{
	static const gchar *suffixes[] = {".mp3", ".ogg", "wav", ".wma", ".aac", ".flac", ".m4a", NULL };
	gchar *lc_name;
	gint i;
	gboolean ret = FALSE;

	// lc_name = g_utf8_strdown(uri, -1);
	lc_name = g_ascii_strdown(uri, -1);
	for (i = 0; suffixes[i] != NULL; i++)
	{
		if (g_str_has_suffix(lc_name, suffixes[i]))
		{
			ret = TRUE;
			break;
		}
	}

	g_free(lc_name);
	return ret;
}

gboolean
eina_file_utils_is_supported_file(GFile *uri)
{
	gboolean ret;
	gchar *u = g_file_get_uri(uri);
	ret = eina_file_utils_is_supported_extension(u);
	g_free(u);
	return ret;
}

