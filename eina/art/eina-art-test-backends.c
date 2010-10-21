/*
 * eina/art/eina-art-test-backends.c
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
#include "eina-art-test-backends.h"
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

void
eina_art_null_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data)
{
	g_warning("Null backend here, failing!");
	eina_art_backend_finish(backend, search);
}

void
eina_art_random_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data)
{
	gboolean s = g_random_boolean();
	g_warning("Random backend here: %s!", s ? "success" : "faling");

	if (s)
		eina_art_search_set_result(search, (gpointer) g_strdup("0xdeadbeef"));

	eina_art_backend_finish(backend, search);
}

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
eina_art_infolder_sync_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data)
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

	LomoStream *stream = eina_art_search_get_stream(search);
	const gchar *uri = lomo_stream_get_tag(stream, LOMO_TAG_URI);

	// Check is stream is local
	gchar *scheme = g_uri_parse_scheme(uri);
	if (!g_str_equal(scheme, "file"))
	{
		g_warning("sync search using coverplus-infolder only works in local files");
		g_free(scheme);
		eina_art_backend_finish(backend, search);
		return;
	}
	g_free(scheme);

	gchar *baseuri = g_path_get_dirname(uri);

	// Try to get a list of folder contents
	GError *error = NULL;
	gchar *dirname = g_filename_from_uri(baseuri, NULL, NULL);
	g_free(baseuri);

	GList *children = gel_dir_read(dirname, FALSE, &error);
	if (error)
	{
		g_warning("Error reading %s: %s", dirname, error->message);
		g_free(dirname);
		g_error_free(error);
		eina_art_backend_finish(backend, search);
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
		GdkPixbuf *pb = gdk_pixbuf_new_from_file(cover_pathname, NULL);
		if (pb && GDK_IS_PIXBUF(pb))
			eina_art_search_set_result(search, pb);
		g_free(cover_pathname);
	}

	// Free used data
	gel_list_deep_free(children, g_free);
	g_free(dirname);

	eina_art_backend_finish(backend, search);
}

