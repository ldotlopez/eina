/*
 * eina/ext/eina-file-utils.c
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

#include "eina-file-utils.h"

/*
 * eina_file_utils_is_supported_extension:
 * @uri: URI to check for support
 *
 * Checks if URI is supported. Test its done using extension.
 *
 * Return: %TRUE if supported, %FALSE otherwise.
 */
gboolean
eina_file_utils_is_supported_extension(gchar *uri)
{
	g_return_val_if_fail(uri != NULL, FALSE);

	static const gchar *suffixes[] = {".mp3", ".ogg", "wav", ".wma", ".aac", ".flac", ".m4a", NULL };

	gchar *lc_name = g_ascii_strdown(uri, -1);
	for (guint i = 0; suffixes[i] != NULL; i++)
	{
		if (g_str_has_suffix(lc_name, suffixes[i]))
		{
			g_free(lc_name);
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * eina_file_utils_is_supported_file:
 * @file: A #GFile to check
 *
 * Checks if the file is supported. This function is a wrapper around
 * eina_file_utils_is_supported_extension(), see doc for details.
 *
 * Returns: %TRUE if supported, %FALSE otherwise.
 */
gboolean
eina_file_utils_is_supported_file(GFile *file)
{
	g_return_val_if_fail(G_IS_FILE(file), FALSE);

	gchar *uri = g_file_get_uri(file);
	g_return_val_if_fail(uri != NULL, FALSE);

	gboolean ret = eina_file_utils_is_supported_extension(uri);
	g_free(uri);

	return ret;
}

