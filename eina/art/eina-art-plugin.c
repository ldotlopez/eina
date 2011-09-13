/*
 * eina/art/eina-art-plugin.c
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

#include "eina/art/eina-art-plugin.h"
#include "eina-art-test-backends.h"
#include <eina/lomo/eina-lomo-plugin.h>

#define DEBUG 0
#define PREFIX "EinaArtPlugin"
#if DEBUG
#	define debug(...) g_debug(PREFIX " " __VA_ARGS__)
#else
#	define debug(...) ;
#endif

enum {
	DEFAULT_COVER_PATH,
	LOADING_COVER_PATH,
	DEFAULT_COVER_URI,
	LOADING_COVER_URI,

	COVER_N_STRINGS
};

static gchar *cover_strings[COVER_N_STRINGS] = { NULL };
EinaArtBackend *null_backend, *infolder_backend;

static void art_search_cb(EinaArtSearch *search, gpointer data);
static void change_cb    (LomoPlayer *lomo, gint from, gint to, EinaArt *art);
static void all_tags_cb  (LomoPlayer *lomo, LomoStream *stream, EinaArt *art);

typedef struct {
	gpointer dummy;
} EinaArtPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_ART_PLUGIN, EinaArtPlugin, eina_art_plugin)

static gboolean
eina_art_plugin_activate(EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaArt *art = eina_art_new();

	EinaArtClass *art_class = EINA_ART_CLASS(G_OBJECT_GET_CLASS(art));

	cover_strings[DEFAULT_COVER_PATH] = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, "cover-default.png");
	cover_strings[LOADING_COVER_PATH] = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, "cover-loading.png");

	if (!cover_strings[DEFAULT_COVER_PATH])
		g_warning(_("Unable to locate '%s'"), "cover-default.png");
	else
		cover_strings[DEFAULT_COVER_URI] = g_filename_to_uri(cover_strings[DEFAULT_COVER_PATH], NULL, NULL);

	if (!cover_strings[LOADING_COVER_PATH])
		g_warning(_("Unable to locate '%s'"), "cover-loading.png");
	else
		cover_strings[LOADING_COVER_URI] = g_filename_to_uri(cover_strings[LOADING_COVER_PATH], NULL, NULL);

	infolder_backend = eina_art_class_add_backend(art_class,
                                              "infolder",
                                              eina_art_infolder_sync_backend_search, NULL,
                                              NULL, NULL);
	eina_application_set_interface(app, "art", art);

	LomoPlayer *lomo = eina_application_get_lomo(app);
	g_signal_connect(lomo, "change",   (GCallback) change_cb,   art);
	g_signal_connect(lomo, "all-tags", (GCallback) all_tags_cb, art);

	return TRUE;
}

G_MODULE_EXPORT gboolean
eina_art_plugin_deactivate(EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaArt *art = eina_application_steal_interface(app, "art");
	EinaArtClass *art_class = EINA_ART_CLASS(G_OBJECT_GET_CLASS(art));

	for (guint i = 0; i < COVER_N_STRINGS; i++)
		gel_free_and_invalidate(cover_strings[i], NULL, g_free);

	eina_art_class_remove_backend(art_class, infolder_backend);

	g_object_unref(art);

	return TRUE;
}

/**
 * eina_application_get_art:
 * @application: An #EinaApplication
 *
 * Get an #EinaArt from #EinaApplication
 *
 * Returns: (transfer none): An #EinaArt
 */
EinaArt*
eina_application_get_art(EinaApplication *application)
{
	static EinaArt *art = NULL;
	if (!art)
		art = eina_art_new();
	return art;
}

/**
 * eina_art_plugin_init_stream:
 * @art: An #EinaArt
 * @stream: (transfer none): A #LomoStream to init
 *
 * Search art data for @stream. @stream while emit the
 * LomoStream:extended-metadata-updated signal when finished
 */
void
eina_art_plugin_init_stream(EinaArt *art, LomoStream *stream)
{
	g_return_if_fail(EINA_IS_ART(art));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	gpointer t = g_object_get_data((GObject *) stream, "art-uri-searched");
	if (t != NULL)
	{
		debug("Stream already initialized, return");
		return;
	}

	const gchar *art_uri = lomo_stream_get_extended_metadata(stream, "art-uri");
	debug("Init stream %p with art-uri: %s", stream, art_uri);
	if (art_uri)
	{
		if (g_str_equal(art_uri, eina_art_plugin_get_loading_cover_uri()))
		{
			debug("Stream %p already has art-uri and it's loading cover.", stream);
			return;
		}
		if (!g_str_equal(art_uri, eina_art_plugin_get_default_cover_uri()))
		{
			debug("Stream %p has custom art-uri: %s", stream, art_uri);
			return;
		}
	}

	art_uri = NULL;
	if (lomo_stream_get_all_tags_flag(stream))
	{
		EinaArtSearch *search = eina_art_search(art, stream, art_search_cb, NULL);
		debug("Stream %p has all_tags flag, search started %p", stream, search);
		if (search)
			art_uri = (gchar *) eina_art_plugin_get_loading_cover_uri();
	}
	else
	{
		debug("Stream %p doesnt have all_tags flags, wait for it. default cover is set", stream);
		art_uri = (gchar *) eina_art_plugin_get_default_cover_uri();
	}

	if (art_uri)
	{
		lomo_stream_set_extended_metadata(stream, "art-uri", (gpointer) art_uri, NULL);
		g_object_set_data((GObject *) stream, "art-uri-searched", (gpointer) TRUE);
	}
}

/**
 * eina_art_plugin_get_default_cover_path:
 *
 * Get the path for the default cover art.
 *
 * Returns: The path
 */
const gchar *
eina_art_plugin_get_default_cover_path()
{
	return cover_strings[DEFAULT_COVER_PATH];
}

/**
 * eina_art_plugin_get_default_cover_uri:
 *
 * Get the URI for the default cover art.
 *
 * Returns: The URI
 */
const gchar *
eina_art_plugin_get_default_cover_uri()
{
	return cover_strings[DEFAULT_COVER_URI];
}

/**
 * eina_art_plugin_get_loading_cover_path:
 *
 * Get the path for the loading cover art
 *
 * Returns: The path
 */
const gchar *
eina_art_plugin_get_loading_cover_path()
{
	return cover_strings[LOADING_COVER_PATH];
}

/**
 * eina_art_plugin_get_loading_cover_uri:
 *
 * Get the URI for the loading cover URI.
 *
 * Returns: The URI
 */
const gchar *
eina_art_plugin_get_loading_cover_uri()
{
	return cover_strings[LOADING_COVER_URI];
}

/*
 * Callbacks
 */
static void
art_search_cb(EinaArtSearch *search, gpointer data)
{
	const gchar *res = eina_art_search_get_result(search);
	LomoStream *stream = eina_art_search_get_stream(search);
	#if DEBUG
	debug ("Got result for stream %p: %p", stream, res);
	#endif

	if (res)
		lomo_stream_set_extended_metadata(eina_art_search_get_stream(search),
			"art-uri", (gpointer) g_strdup(res), g_free);
	else
		lomo_stream_set_extended_metadata(eina_art_search_get_stream(search),
			"art-uri", cover_strings[DEFAULT_COVER_URI] , NULL);

	g_object_set_data((GObject *) stream, "art-uri-searched", (gpointer) TRUE);

	debug("Extended metadata set to: %s",
		(gchar *) lomo_stream_get_extended_metadata(eina_art_search_get_stream(search), "art-uri"));
}

static void
change_cb(LomoPlayer *lomo, gint from, gint to, EinaArt *art)
{
	g_return_if_fail(LOMO_IS_PLAYER(lomo));
	g_return_if_fail(EINA_IS_ART(art));

	if (to == -1)
		return;

	LomoStream *streams[2] = {
		lomo_player_get_nth_stream(lomo, to),
		lomo_player_get_nth_stream(lomo, lomo_player_get_next(lomo)) };

	for (guint i = 0; i < G_N_ELEMENTS(streams); i++)
	{
		if (!streams[i])
			continue;

		eina_art_plugin_init_stream(art, streams[i]);
	}
}

static void
all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaArt *art)
{
	const gchar *stream_art_uri = lomo_stream_get_extended_metadata(stream, "art-uri");
	if (g_strcmp0(stream_art_uri, eina_art_plugin_get_default_cover_uri()) == 0)
	{
		debug("Catch a stream all-tags signal for stream with default cover");
		eina_art_plugin_init_stream(art, stream);
	}
}

