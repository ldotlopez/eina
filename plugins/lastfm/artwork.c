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
	GHashTable *searches;
} ;

typedef struct {
	LastFM      *self;
	gint         search;
	LomoStream  *stream;
	EinaArtwork *artwork;
} SearchCtx;

typedef void (*SearchFunc)(SearchCtx *ctx);

static void
lastfm_artwork_search(EinaArtwork *artwork, LomoStream *stream, LastFM *self);
static void
lastfm_artwork_cancel(EinaArtwork *artwork, LastFM *self);

static void
search(SearchCtx *ctx);
static void
search_by_album (SearchCtx *ctx);
static void
search_by_artist(SearchCtx *ctx);
static void
search_by_single(SearchCtx *ctx);

gboolean
lastfm_artwork_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	EINA_PLUGIN_DATA(plugin)->artwork = g_new0(LastFMArtwork, 1);
	// EINA_PLUGIN_DATA(plugin)->artwork->searches = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, search_ctx_free);

	eina_plugin_add_artwork_provider(plugin, "lastfm",
		(EinaArtworkProviderSearchFunc) lastfm_artwork_search,
		(EinaArtworkProviderCancelFunc) lastfm_artwork_cancel,
		NULL);

	gel_warn("Artwork init!");

	return TRUE;
}

gboolean
lastfm_artwork_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	eina_plugin_remove_artwork_provider(plugin, "lastfm");

	// g_hash_table_destroy(EINA_PLUGIN_DATA(plugin)->artwork->searches);
	g_free              (EINA_PLUGIN_DATA(plugin)->artwork);

	gel_warn("Artwork fini!");
	return TRUE;
}

SearchCtx*
search_ctx_new(LomoStream *stream, EinaArtwork *artwork)
{
	SearchCtx *ctx = g_new0(SearchCtx, 1);
	ctx->stream  = stream;
	ctx->artwork = artwork;
	ctx->search  = 0;
	return ctx;
}

void
search_ctx_free(SearchCtx *ctx)
{
	g_free(ctx);
}

void
lastfm_artwork_search(EinaArtwork *artwork, LomoStream *stream, LastFM *self)
{
	gel_warn("Start search for %s", lomo_stream_get_tag(stream, LOMO_TAG_URI));
	SearchCtx *ctx = g_new0(SearchCtx, 1);
	ctx->self    = self;
	ctx->search  = 0;
	ctx->stream  = stream;
	ctx->artwork = artwork;

	search(ctx);
}

void
lastfm_artwork_cancel(EinaArtwork *artwork, LastFM *self)
{

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
	if (searches[ctx->search] == NULL)
	{
		eina_artwork_provider_fail(ctx->artwork);
		g_free(ctx);
	}
	else
		searches[ctx->search](ctx);
}

static void
search_by_album(SearchCtx *ctx)
{
	gel_warn("Search by album %p fail", ctx);
	ctx->search++;
	search(ctx);
}

static void
search_by_artist(SearchCtx *ctx)
{
	gel_warn("Search by artist %p fail", ctx);
	ctx->search++;
	search(ctx);
}

static void
search_by_single(SearchCtx *ctx)
{
	gel_warn("Search by single %p fail", ctx);
	ctx->search++;
	search(ctx);
}
