/*
 * lomo/lomo-em-art-backends.c
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
#include "lomo-em-art-backends.h"
#include <glib/gi18n.h>
#include <gel/gel.h>

// --
// infolder-sync backend
// --
static gchar *_infolder_regexes_str[] = {
	".*front.*\\.(jpe?g|png|gif)$",
	".*cover.*\\.(jpe?g|png|gif)$",
	".*folder.*\\.(jpe?g|png|gif)$",
	".*\\.(jpe?g|png|gif)$",
	NULL
};
static GRegex *_infolder_regexes[4] = { NULL };

void
lomo_em_art_infolder_sync_backend_search(LomoEMArtBackend *backend, LomoEMArtSearch *search, gpointer data)
{
	// Create regexes
	if (_infolder_regexes[0] == NULL)
	{
		for (guint i = 0; _infolder_regexes_str[i] != NULL; i++)
		{
			GError *error = NULL;
			_infolder_regexes[i] = g_regex_new(_infolder_regexes_str[i],
				G_REGEX_CASELESS|G_REGEX_DOTALL|G_REGEX_DOLLAR_ENDONLY|G_REGEX_OPTIMIZE|G_REGEX_NO_AUTO_CAPTURE,
				0, &error);
			if (!_infolder_regexes[i])
			{
				g_warning(N_("Unable to compile regex '%s': %s"), _infolder_regexes_str[i], error->message);
				g_error_free(error);
			}
		}
	}

	LomoStream *stream = lomo_em_art_search_get_stream(search);
	const gchar *uri = lomo_stream_get_uri(stream);

	if (g_str_equal(uri,"file:///nonexistent"))
	{
		lomo_em_art_backend_finish(backend, search);
		return;
	}

	// Check is stream is local
	gchar *scheme = g_uri_parse_scheme(uri);
	if (!g_str_equal(scheme, "file"))
	{
		g_warning("sync search using coverplus-infolder only works in local files");
		g_free(scheme);
		lomo_em_art_backend_finish(backend, search);
		return;
	}
	g_free(scheme);

	gchar *filename = g_filename_from_uri(uri, NULL, NULL);
	gchar *dirname = g_path_get_dirname(filename);
	g_free(filename);

	// Try to get a list of folder contents
	GError *error = NULL;
	GList *children = gel_dir_read(dirname, FALSE, &error);
	if (!children)
	{
		g_free(dirname);
		lomo_em_art_backend_finish(backend, search);
		return;
	}

	GList *iter = children;
	gchar *winner = NULL;
	gint score = G_MAXINT;
	while (iter)
	{
		for (guint i = 0; i < G_N_ELEMENTS(_infolder_regexes); i++)
		{
			if (_infolder_regexes[i] && g_regex_match(_infolder_regexes[i], (gchar *) iter->data, 0, NULL) && (score > i))
			{
				winner = iter->data;
				score = i;
			}
		}
		iter = iter->next;
	}

	if (score < G_MAXINT)
	{
		gchar *cover_pathname = g_build_filename(dirname, winner, NULL);
		gchar *cover_uri = g_filename_to_uri(cover_pathname, NULL, NULL);
		if (cover_uri)
			lomo_em_art_search_set_result(search, cover_uri);
		g_free(cover_pathname);
		g_free(cover_uri);
	}

	// Free used data
	gel_list_deep_free(children, g_free);
	g_free(dirname);

	lomo_em_art_backend_finish(backend, search);
}

