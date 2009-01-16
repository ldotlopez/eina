/*
 * plugins/coverplus/banshee.c
 *
 * Copyright (C) 2004-2009 Eina
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

#include "banshee.h"

// --
// Banshee covers
// --
void
coverplus_banshee_search_cb(EinaArtwork *cover, LomoStream *stream, gpointer data)
{
	GString *str;
	gint i, j;
	gchar *input[3] = {
		g_utf8_strdown(lomo_stream_get_tag(stream, LOMO_TAG_ARTIST), -1),
		g_utf8_strdown(lomo_stream_get_tag(stream, LOMO_TAG_ALBUM), -1),
		NULL
	};

	str = g_string_new(NULL);

	for (i = 0; input[i] != NULL; i++)
	{
		for (j = 0; input[i][j] != '\0'; j++)
		{
			if (g_ascii_isalnum(input[i][j]))
				str = g_string_append_c(str, input[i][j]);
		}
		if (i == 0)
			str = g_string_append_c(str, '-');
		g_free(input[i]);
	}
	str = g_string_append(str, ".jpg");

	gchar *paths[2];
	paths[0] = g_build_filename(g_get_home_dir(), ".config", "banshee", "covers", str->str, NULL);
	paths[1] = g_build_filename(g_get_home_dir(), ".cache", "album-art", str->str, NULL);
	paths[2] = NULL;
	g_string_free(str, TRUE);

	gboolean found = FALSE;
	for (i = 0; paths[i] != NULL; i++)
	{
		if (g_file_test(paths[i], G_FILE_TEST_IS_REGULAR|G_FILE_TEST_EXISTS))
		{
			eina_artwork_provider_success(cover, G_TYPE_STRING, paths[i]);
			found = TRUE;
			break;
		}
	}

	for (i = 0; paths[i] != NULL; i++)
		g_free(paths[i]);

	if (!found);
		eina_artwork_provider_fail(cover);
		
}

