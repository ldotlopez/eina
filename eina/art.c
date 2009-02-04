#include "eina/art.h"

struct _Art {
	GList *backends;
	GList *searches;
};

struct _ArtBackend {
	Art     *art;
	ArtFunc  search, cancel;
	GList   *searches;
	gpointer data;
	gboolean enabled;
};

struct _ArtSearch {
	Art *art;
	LomoStream *stream;
	ArtFunc success, fail;
	gpointer data;
	GList *backend_link;
	gboolean completed;
};

// --
// Art
// --
/*
static void
art_run_success(Art *art, ArtSearch *search);
*/
static void
art_run_fail(Art *art, ArtSearch *search);


// --
// Backend
// --
static void
art_backend_run_search(ArtBackend *backend, ArtSearch *search);

// --
// Search
// --
static ArtSearch*
art_search_new(Art *art, LomoStream *stream, ArtFunc success_func, ArtFunc fail_func, gpointer data);
static void
art_search_destroy(ArtSearch *search);


// --
// Art
// --
Art*
art_new(void)
{
	Art *art = g_new0(Art, 1);
	return art;
}

void
art_destroy(Art *art)
{
	// Destroy all search to prevent they jump over backends with no profit
	GList *iter = art->searches;
	while (iter)
	{
		art_search_destroy((ArtSearch*) iter->data);
		iter = iter->next;
	}

	while (iter)
	{
		art_backend_destroy((ArtBackend *) iter->data); // Also destroys searches
		iter = iter->next;
	}
	// XXX: Stop all backends
	// XXX: Destroy all searches
	g_free(art);
}

ArtSearch*
art_search(Art *art, LomoStream *stream, ArtFunc success_func, ArtFunc fail_func, gpointer data)
{
	ArtSearch *search = art_search_new(art, stream, success_func, fail_func, data);
	
	if (art->backends == NULL)
	{
		art_run_fail(art, search);
	}

	// Set backend in stream
	// art_search_set_backend_link(search, art->backends);

	// XXX: Inject into first backend
	// Start processing

	return search;
}

void
art_cancel(Art *art, ArtSearch *search)
{
	// Emit cancel
	// Delete from backends
	art_search_destroy(search);
}

/*
static void
art_run_success(Art *art, ArtSearch *search)
{
	if (search->success)
		search->success(art, search, search->data);
	search->completed = TRUE;
	// XXX: Deattach from backend
}
*/

static void
art_run_fail(Art *art, ArtSearch *search)
{
	if (search->fail)
		search->fail(art, search, search->data);
	search->completed = TRUE;
	// XXX: Deattach from backend
}

static void
art_forward_search(Art *art, ArtSearch *search)
{
	if ((search->backend_link = search->backend_link->next) == NULL)
	{
		art_run_fail(art, search);
		art_search_destroy(search);
		return;
	}

	art_backend_run_search(search->backend_link->data, search);
}

// --
// Backend
// --
ArtBackend *
art_backend_new(Art *art, ArtFunc search_func, ArtFunc cancel_func, gpointer data)
{
	ArtBackend *backend = g_new0(ArtBackend, 1);
	backend->art      = art;
	backend->search   = search_func;
	backend->cancel   = cancel_func;
	backend->data     = data;
	backend->searches = NULL;
	backend->enabled  = TRUE; // Ready to run 
	return backend;
}

static void
art_backend_disable(ArtBackend *backend)
{
	backend->enabled = FALSE;

	GList *iter = backend->searches;
	while (iter)
	{
		art_forward_search(backend->art, (ArtSearch*) iter->data);
		iter = iter->next;
	}
	g_list_free(backend->searches);
	backend->searches = NULL;
}

void
art_backend_destroy(ArtBackend *backend)
{
	if (backend->enabled)
		art_backend_disable(backend);
	g_free(backend);
}

static void
art_backend_run_search(ArtBackend *backend, ArtSearch *search)
{
	// XXX: Add this search to our list
	// XXX: Check ownership of search
	if (backend->search == NULL)
		art_forward_search(backend->art, search);
	backend->search(backend->art, search, backend->data);
}

static ArtSearch*
art_search_new(Art *art, LomoStream *stream, ArtFunc success_func, ArtFunc fail_func, gpointer data)
{
	ArtSearch *search = g_new0(ArtSearch, 1);
	search->art     = art;
	search->stream  = stream;
	search->success = success_func;
	search->fail    = fail_func;
	search->data    = data;
	search->backend_link = NULL;
	search->completed = FALSE;
	return search;
}

static void
art_search_destroy(ArtSearch *search)
{
	if (!search->completed)
		art_run_fail(search->art, search);
	// XXX: Cancel if not cancelled
	// XXX: Remove from any backend
	g_free(search);
}
