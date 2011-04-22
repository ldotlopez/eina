/*
 * eina/art/art.c
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

#include "art.h"
#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "eina-art-test-backends.h"

EinaArtBackend *null_backend, *infolder_backend;

enum
{
	DEFAULT_COVER_PATH,
	LOADING_COVER_PATH,
	DEFAULT_COVER_URI,
	LOADING_COVER_URI,

	COVER_N_STRINGS
};
static gchar *cover_strings[COVER_N_STRINGS] = { NULL };

static void
insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaArt *art);
static void
art_search_cb(EinaArtSearch *search, gpointer data);

void
eina_art_plugin_init_stream(EinaArt *art, LomoStream *stream)
{
	EinaArtSearch *search = NULL;
	if (lomo_stream_get_all_tags_flag(stream) && (search = eina_art_search(art, stream, art_search_cb, NULL)))
		lomo_stream_set_extended_metadata(stream, "art-uri", (gpointer) cover_strings[LOADING_COVER_URI], NULL);
	else
		lomo_stream_set_extended_metadata(stream, "art-uri", (gpointer) cover_strings[DEFAULT_COVER_URI], NULL);
}

const gchar *
eina_art_plugin_get_default_cover_path()
{
	return cover_strings[DEFAULT_COVER_PATH];
}

const gchar *
eina_art_plugin_get_default_cover_uri()
{
	return cover_strings[DEFAULT_COVER_URI];
}

const gchar *
eina_art_plugin_get_loading_cover_path()
{
	return cover_strings[LOADING_COVER_PATH];
}

const gchar *
eina_art_plugin_get_loading_cover_uri()
{
	return cover_strings[LOADING_COVER_URI];
}

static void
art_search_cb(EinaArtSearch *search, gpointer data)
{
	const gchar *res = eina_art_search_get_result(search);
	// g_warning(_("Search finished: %s"), res);
	if (res)
		lomo_stream_set_extended_metadata(eina_art_search_get_stream(search), "art-uri", (gpointer) g_strdup(res), g_free);
	else
		lomo_stream_set_extended_metadata(eina_art_search_get_stream(search), "art-uri", cover_strings[DEFAULT_COVER_URI] , NULL);
}

static void
insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaArt *art)
{
	eina_art_plugin_init_stream(art, stream);
}

static void
all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaArt *art)
{
	EinaArtSearch *search = eina_art_search(art, stream, art_search_cb, NULL);
	if (search)
	{
		// g_warning(_("all-tags Search started: %p, setting loading cover"), search);
		lomo_stream_set_extended_metadata(stream, "art-uri", (gpointer) cover_strings[LOADING_COVER_URI], NULL);
	}
	else
	{
		// g_warning(_("alkl-tags Search doesnt started, set default cover"));
		lomo_stream_set_extended_metadata(stream, "art-uri", (gpointer) cover_strings[DEFAULT_COVER_URI], NULL);
	}
}

G_MODULE_EXPORT gboolean
art_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
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


	null_backend = eina_art_class_add_backend(art_class,
                                              "null",
                                              eina_art_null_backend_search, NULL,
                                              NULL, NULL);
	infolder_backend = eina_art_class_add_backend(art_class,
                                              "infolder",
                                              eina_art_infolder_sync_backend_search, NULL,
                                              NULL, NULL);
	eina_application_set_interface(app, "art", art);

	LomoPlayer *lomo = eina_application_get_lomo(app);
	g_signal_connect(lomo, "insert",   (GCallback) insert_cb,   art);
	g_signal_connect(lomo, "all-tags", (GCallback) all_tags_cb, art);

	return TRUE;
}

G_MODULE_EXPORT gboolean
art_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	EinaArt *art = eina_application_steal_interface(app, "art");
	EinaArtClass *art_class = EINA_ART_CLASS(G_OBJECT_GET_CLASS(art));

	for (guint i = 0; i < COVER_N_STRINGS; i++)
		gel_free_and_invalidate(cover_strings[i], NULL, g_free);

	eina_art_class_remove_backend(art_class, null_backend);
	eina_art_class_remove_backend(art_class, infolder_backend);
	g_object_unref(art);

	return TRUE;
}
