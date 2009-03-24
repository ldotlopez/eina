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

#define USE_CURL 1

#include "artwork.h"
#if USE_CURL
#include "curl-engine.h"
#endif

// ****************************
// Store artwork-subplugin data
// ****************************
struct _LastFMArtwork {
	ArtBackend *backend; // Reference the backend
	GHashTable *data;    // SearchCtx's
#if USE_CURL
	CurlEngine *engine;  // CurlEngine C-class
#endif
};

// *********************************
// SearchCtx is a ArtSearch extended
// *********************************
typedef struct {
	Art          *art;         // Art reference
	ArtSearch    *search;      // Art search
	gint          n;           // sub-search index
#if USE_CURL
	CurlEngine   *engine;      // Inet engine
	CurlQuery    *q;           // This query
#else
	GCancellable *cancellable; // GCancellable object
#endif
} SearchCtx;

// **********************
// Sub-searches prototype
// **********************
typedef void (*SearchFunc)(SearchCtx *ctx);

// ******************
// Internal functions
// ******************
static void lastfm_artwork_search(Art *art, ArtSearch *search, LastFMArtwork *self);
static void lastfm_artwork_cancel(Art *art, ArtSearch *search, LastFMArtwork *self);

static SearchCtx* search_ctx_new   (LastFMArtwork *self, Art *art, ArtSearch *search);
static void       search_ctx_free  (SearchCtx *ctx);
static void       search_ctx_search(SearchCtx *ctx); 
static void       search_ctx_cancel(SearchCtx *ctx); 

static void search_ctx_by_album(SearchCtx *ctx);
static void search_ctx_by_artist(SearchCtx *ctx);

gchar* search_ctx_parse_as_album(gchar *buffer);
gchar* search_ctx_parse_as_artist(gchar *buffer);

#if USE_CURL
static void curl_engine_finish_cb(CurlEngine *engine, CurlQuery *query, SearchCtx *ctx);
static void curl_engine_cover_cb (CurlEngine *engine, CurlQuery *query, SearchCtx *ctx);
#else
#endif

// ******************************************
// Initialize and finalize artwork sub-plugin
// ******************************************
gboolean
lastfm_artwork_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFMArtwork *self = EINA_PLUGIN_DATA(plugin)->artwork = g_new0(LastFMArtwork, 1);

#if USE_CURL
	if ((self->engine = curl_engine_new()) == NULL)
	{
		gel_warn("Cannit init curl interface");
		g_free(self);
		return FALSE;
	}
#endif

	self->data    = g_hash_table_new(g_direct_hash, g_direct_equal);
	self->backend = eina_plugin_add_art_backend(plugin, "lastfm",
		(ArtFunc) lastfm_artwork_search, (ArtFunc) lastfm_artwork_cancel,
		self);

	return TRUE;
}

gboolean
lastfm_artwork_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFMArtwork *self = EINA_PLUGIN_DATA(plugin)->artwork;

#if USE_CURL
	curl_engine_free(self->engine);
	g_hash_table_destroy(self->data);
#endif

	eina_plugin_remove_art_backend(plugin, self->backend);
	g_free(self);

	return TRUE;
}

// ********************
// ArtBackend functions
// ********************
void
lastfm_artwork_search(Art *art, ArtSearch *search, LastFMArtwork *self)
{
	SearchCtx *ctx = search_ctx_new(self, art, search);
	if (ctx == NULL)
	{
		gel_warn("Cannot create search context");
		art_report_failure(art, search);
	}
	else
	{
		g_hash_table_replace(self->data, search, ctx);
		search_ctx_search(ctx);
	}
}

void
lastfm_artwork_cancel(Art *art, ArtSearch *search, LastFMArtwork *self)
{
	SearchCtx *ctx = g_hash_table_lookup(self->data, search);
	g_return_if_fail(ctx);

	gel_warn("Cancel search %p", ctx);
	search_ctx_cancel(ctx);
	search_ctx_free(ctx);

	g_hash_table_remove(self->data, search);
}

// **********************************
// Creating and destroying SearchCtxs
// **********************************
SearchCtx*
search_ctx_new(LastFMArtwork *self, Art *art, ArtSearch *search)
{
	SearchCtx *ctx = g_new0(SearchCtx, 1);

	ctx->art    = art;
	ctx->search = search;
	ctx->n      = 0;
#if USE_CURL
	ctx->engine = self->engine;
	ctx->q      = NULL;
#else
	ctx->cancellable = g_cancellable_new();
#endif

	return ctx;
}

void
search_ctx_free(SearchCtx *ctx)
{
	if (ctx->q != NULL)
		curl_engine_cancel(ctx->engine, ctx->q);

#if !USE_CURL
	g_object_unref(ctx->cancellable);
#endif

	g_free(ctx);
}

// ***************************
// Search and cancel functions
// ***************************
void
search_ctx_search(SearchCtx *ctx)
{
	// subsearches
	static SearchFunc subsearches[] = {
		search_ctx_by_album,
		search_ctx_by_artist,
		// search_ctx_by_single,
		NULL
	};

	// Check current index and launch search
	if (subsearches[ctx->n] == NULL)
	{
		art_report_failure(ctx->art, ctx->search);
		search_ctx_free(ctx);
	}
	else
	{
		subsearches[ctx->n](ctx);
	}
}

void
search_ctx_cancel(SearchCtx *ctx)
{
	if (ctx->q)
	{
		curl_engine_cancel(ctx->engine, ctx->q);
		ctx->q = NULL;
	}
}

void
search_ctx_try_next(SearchCtx *ctx)
{
	ctx->n++;
	search_ctx_search(ctx);
}

gchar*
search_ctx_parse(SearchCtx *ctx, gchar *buffer)
{
	static gpointer parsers[] = {
		search_ctx_parse_as_album,
		search_ctx_parse_as_artist,
		// search_ctx_parse_as_single,
		NULL
	};

	gchar *ret = NULL;
	if (parsers[ctx->n] != NULL)
	{
		gchar* (*parser) (gchar *buffer) = parsers[ctx->n];
		ret = parser(buffer);
	}
	return ret;
}

// ***********
// Subsearches
// ***********
static void
search_ctx_by_album(SearchCtx *ctx)
{
	LomoStream *stream = art_search_get_stream(ctx->search);
	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	gchar *album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);

	// search_by_album needs artist and album tags
	if (!artist || !album)
	{
		search_ctx_try_next(ctx);
		return;
	}

	// Now control belongs to curl_engine_query_cb / load_contents_async_cb
	gchar *a, *b;
	a = g_uri_escape_string(artist, G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS, FALSE);
	b = g_uri_escape_string(album, G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS, FALSE);
	gchar *uri = g_strdup_printf("http://www.last.fm/music/%s/%s", a, b);
	g_free(a);
	g_free(b);

#if USE_CURL
	ctx->q = curl_engine_query(ctx->engine, uri, (CurlEngineFinishFunc) curl_engine_finish_cb, ctx);
#else
	g_file_load_contents_async(g_file_new_for_uri(uri), ctx->cancellable,
		(GAsyncReadyCallback) load_contents_async_cb, ctx);
#endif

	g_free(uri);
}

static void
search_ctx_by_artist(SearchCtx *ctx)
{
	LomoStream *stream = art_search_get_stream(ctx->search);
	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);

	// search_by_artist needs artist tag
	if (!artist)
	{
		search_ctx_try_next(ctx);
		return;
	}

	// Now control belongs to curl_engine_query_cb / load_contents_async_cb
	gchar *a   = g_uri_escape_string(artist, G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS, FALSE);
	gchar *uri = g_strdup_printf("http://www.last.fm/music/%s", a);
	g_free(a);

#if USE_CURL
	ctx->q = curl_engine_query(ctx->engine, uri, (CurlEngineFinishFunc) curl_engine_finish_cb, ctx);
#else
	g_file_load_contents_async(g_file_new_for_uri(uri), ctx->cancellable,
		(GAsyncReadyCallback) load_contents_async_cb, ctx);
#endif

	g_free(uri);
}


// *******
// SubParsers
// *******
gchar *
search_ctx_parse_as_album(gchar *buffer)
{
	gchar *tokens[] = {
		"<span class=\"art\"><img",
		"src=\"",
		NULL
	};

	gint i;
	gchar *p = buffer;
	for (i = 0; tokens[i] != NULL; i++)
	{
		if ((p = strstr(p, tokens[i])) == NULL)
			return NULL;
		p += strlen(tokens[i]) * sizeof(gchar);
	}

	gchar *p2 = strstr(p, "\"");
	if (!p2)
		return NULL;
	p2[0] = '\0';
	
	if (g_str_has_suffix(p, "default_album_mega.png"))
		return NULL;

	return g_strdup(p);
}

gchar *
search_ctx_parse_as_artist(gchar *buffer)
{
	gchar *tokens[] = {
		"id=\"catalogueImage\"",
		"src=\"",
		NULL
	};

	gint i;
	gchar *p = buffer;
	for (i = 0; tokens[i] != NULL; i++)
	{
		if (!p || (p = strstr(p, tokens[i])) == NULL)
			return NULL;
		p += strlen(tokens[i]) * sizeof(gchar);
	}

	gchar *p2 = strstr(p, "\"");
	if (!p2)
		return NULL;
	p2[0] = '\0';
	
	if (g_str_has_suffix(p, "default_album_mega.png"))
		return NULL;

	return g_strdup(p);
}
// ******************
// Callback functions
// ******************
#if USE_CURL

static void
curl_engine_finish_cb(CurlEngine *engine, CurlQuery *query, SearchCtx *ctx)
{
	guint8 *buffer;
	gsize   size;
	GError *error = NULL;

	// Search is finished
	ctx->q = NULL;

	// Jump if there was an error fetching
	if (!curl_query_finish(query, &buffer, &size, &error))
	{
		gchar *uri = curl_query_get_uri(query);
		gel_warn(N_("Cannot fetch uri '%s': %s"), uri, error->message);
		g_free(uri);

		goto curl_engine_finish_cb_fail;
	}

	// Jump if parse failed
	gchar *cover = search_ctx_parse(ctx, (gchar *) buffer);
	if (cover == NULL)
	{
		gel_warn(N_("Parse in stage %d failed for search %p"), ctx->n, ctx);
		goto curl_engine_finish_cb_fail;
	}

	// Ok, fetch the cover
	curl_engine_query(ctx->engine, cover,
		(CurlEngineFinishFunc) curl_engine_cover_cb, ctx);
	g_free(cover);
	return;

curl_engine_finish_cb_fail:
	gel_free_and_invalidate(error,  NULL, g_error_free);
	gel_free_and_invalidate(buffer, NULL, g_free);
	search_ctx_try_next(ctx);
}

static void
curl_engine_cover_cb(CurlEngine *engine, CurlQuery *query, SearchCtx *ctx)
{
	guint8 *buffer;
	gsize   size;
	GError *error = NULL;

	// Jump if there was an error fetching
	if (!curl_query_finish(query, &buffer, &size, &error))
	{
		gchar *uri = curl_query_get_uri(query);
		gel_warn("Cannot get uri '%s': %s", uri, error->message);
		g_error_free(error);
		g_free(uri);

		goto curl_engine_cover_cb_fail;
	}

	// Load pixbuf
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
	if (!gdk_pixbuf_loader_write(loader, buffer, size, &error))
	{
		gdk_pixbuf_loader_close(loader, NULL);
		gel_warn("Cannot load to pixbuf: %s", error->message);
		goto curl_engine_cover_cb_fail;
	}
	gdk_pixbuf_loader_close(loader, NULL);

	GdkPixbuf *pb = gdk_pixbuf_loader_get_pixbuf(loader);
	if (!pb)
		goto curl_engine_cover_cb_fail;

	g_object_ref(pb);
	g_object_unref(loader);

	// Report and cleanup
	ctx->q = NULL;
	art_report_success(ctx->art, ctx->search, (gpointer) pb);
	search_ctx_free(ctx);
	return;

curl_engine_cover_cb_fail:
	gel_free_and_invalidate(loader, NULL, g_object_unref);
	gel_free_and_invalidate(error,  NULL, g_error_free);
	gel_free_and_invalidate(buffer, NULL, g_free);
	search_ctx_try_next(ctx);
}

#else

#endif
