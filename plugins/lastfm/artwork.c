/*
 * plugins/lastfm/artwork.c
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

#include "artwork.h"

struct _LastFMArtwork {
	ArtBackend *backend;
	GHashTable *data;
};

typedef struct {
	Art          *art;
	ArtSearch    *search;
	gint          n;
	GCancellable *cancellable;
} SearchCtx;

typedef void(*SearchFunc)(SearchCtx *ctx);

static void
lastfm_artwork_search(Art *art, ArtSearch *_search, LastFMArtwork *self);
static void
lastfm_artwork_cancel(Art *art, ArtSearch *_search, LastFMArtwork *self);

static void
search(SearchCtx *ctx);
static void
search_by_album(SearchCtx *ctx);
static void
search_by_artist(SearchCtx *ctx);
static void
search_by_single(SearchCtx *ctx);

gboolean
lastfm_artwork_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFMArtwork *self = EINA_PLUGIN_DATA(plugin)->artwork = g_new0(LastFMArtwork, 1);

	self->data = g_hash_table_new(g_direct_hash, g_direct_equal);
	self->backend = eina_plugin_add_art_backend(plugin, "lastfm",
		(ArtFunc) lastfm_artwork_search, (ArtFunc) lastfm_artwork_cancel,
		self);

	gel_warn("Artwork init!");

	return TRUE;
}

gboolean
lastfm_artwork_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFMArtwork *self = EINA_PLUGIN_DATA(plugin)->artwork;

	// XXX: Remove all data search
	g_hash_table_destroy(self->data);

	eina_plugin_remove_art_backend(plugin, self->backend);
	g_free(self);

	gel_warn("Artwork fini!");
	return TRUE;
}

SearchCtx*
search_ctx_new(Art *art, ArtSearch *search)
{
	SearchCtx *ctx = g_new0(SearchCtx, 1);
	ctx->art = art;
	ctx->search = search;
	ctx->n = 0;
	ctx->cancellable = g_cancellable_new();
	return ctx;
}

void
search_ctx_free(SearchCtx *ctx)
{
	g_object_unref(ctx->cancellable);
	g_free(ctx);
}

void
lastfm_artwork_search(Art *art, ArtSearch *_search, LastFMArtwork *self)
{
	LomoStream *stream = art_search_get_stream(_search);

	SearchCtx *ctx = search_ctx_new(art, _search);
	g_hash_table_replace(self->data, _search, ctx);
	gel_warn("%p => %p", _search, ctx->search);

	gel_warn("Start search (%p) for %s", ctx, lomo_stream_get_tag(stream, LOMO_TAG_URI));

	search(ctx);
}

void
lastfm_artwork_cancel(Art *art, ArtSearch *_search, LastFMArtwork *self)
{
	SearchCtx *ctx = g_hash_table_lookup(self->data, _search);
	g_return_if_fail(ctx);

	gel_warn("Cancel search %p", ctx);
}

static void
search(SearchCtx *ctx)
{
	static SearchFunc searches[] = {
		search_by_album,
		search_by_artist,
		search_by_single,
		NULL
	};
	if (searches[ctx->n] == NULL)
	{
		art_report_failure(ctx->art, ctx->search);
		g_free(ctx);
	}
	else
		searches[ctx->n](ctx);
}

static void
search_by_album_cb(GFile *file, GAsyncResult *res, SearchCtx *ctx)
{
	gchar *buff = NULL;
	gsize len;
	GError *err = NULL;

	if (!g_file_load_contents_finish(file, res, &buff, &len, NULL, &err))
	{
		gel_warn("Error loading: %s", err->message);
		goto search_by_album_cb_fail;
	}

	gchar *tokens[] = {
		"<span class=\"art\"><img",
		"src=\"",
		NULL
	};

	gint i;
	gchar *p = buff;
	for (i = 0; tokens[i] != NULL; i++)
	{
		if ((p = strstr(p, tokens[i])) == NULL)
			goto search_by_album_cb_fail;
		p += strlen(tokens[i]) * sizeof(gchar);
	}
	strstr(p, "\"")[0] = '\0';
	gel_warn("Cover: %s", p);

search_by_album_cb_fail:
	gel_free_and_invalidate(file, NULL, g_object_unref);
	gel_free_and_invalidate(err,  NULL, g_error_free);
	gel_free_and_invalidate(buff, NULL, g_free);

	ctx->n++;
	search(ctx);
}

static void
search_by_album(SearchCtx *ctx)
{
	LomoStream *stream = art_search_get_stream(ctx->search);
	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	gchar *album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);

	gel_warn("Search by album %p fail", ctx);

	if (!artist || !album)
	{
		gel_warn("Needed artist(%s) and album(%s)", 
			artist ? "found" : "not found",
			album  ? "found" : "not found");
		ctx->n++;
		search(ctx);
		return;
	}

	gchar *uri = g_strdup_printf("http://www.lastfm.es/music/%s/%s", artist, album);
	gel_warn("Point to %s", uri);
	g_file_load_contents_async(g_file_new_for_uri(uri), ctx->cancellable,
		(GAsyncReadyCallback) search_by_album_cb, ctx);
	// ctx->n++;
	// search(ctx);
}

static void
search_by_artist(SearchCtx *ctx)
{
	gel_warn("Search by artist %p fail", ctx);
	ctx->n++;
	search(ctx);
}

static void
search_by_single(SearchCtx *ctx)
{
	gel_warn("Search by single %p fail", ctx);
	ctx->n++;
	search(ctx);
}
