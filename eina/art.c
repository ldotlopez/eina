#define GEL_DOMAIN "Eina::Art"
#include "eina/art.h"
#include <eina/eina-plugin.h>
#include <config.h>
#define ART_AHEAD 1

struct _Art {
	GList *backends; // updated by art_(add|remove)_backend
	GList *searches; // updated by art_search, art_cancel
};

struct _ArtBackend {
	gchar *name;
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
	gpointer result;
	gpointer data;
	GList *backend_link;
	gboolean running;
};

static void
art_forward_search(Art *art, ArtSearch *search);

void
art_run_search(Art *art, ArtSearch *search);
void
art_run_fail(Art *art, ArtSearch *search);

ArtBackend *
art_backend_new(Art *art, gchar *name, ArtFunc search, ArtFunc cancel, gpointer data);
void
art_backend_destroy(ArtBackend *backend);
void
art_backend_search(ArtBackend *backend, ArtSearch *search);
void
art_backend_cancel(ArtBackend *backend, ArtSearch *search);

ArtSearch*
art_search_new(LomoStream *stream, ArtFunc success, ArtFunc fail, gpointer data);
void
art_search_destroy(ArtSearch *search);

Art*
art_new(void)
{
	Art *art = g_new0(Art, 1);
	return art;
}

void
art_destroy(Art *art)
{
	GList *b = art->backends;
	while (b)
	{
		ArtBackend *backend = (ArtBackend *) b->data;
		GList *s = backend->searches;
		while (s)
		{
			ArtSearch *search = (ArtSearch *) s->data;
			if (search->running)
				art_backend_cancel(backend, search);
			art_search_destroy(search);

			s = s->next;
		}
		art_backend_destroy(backend);

		b = b->next;
	}
	g_list_free(art->searches);
	g_list_free(art->backends);
	g_free(art);
}

// --
// At the beginning of the time we need to add some backends and do some real
// work
// --
ArtBackend *
art_add_backend(Art *art, gchar *name, ArtFunc search, ArtFunc cancel, gpointer data)
{
	ArtBackend *backend = art_backend_new(art, name, search, cancel, data);
	art->backends = g_list_append(art->backends, backend);
	return backend;
}

void
art_remove_backend(Art *art, ArtBackend *backend)
{
	// Disable
	backend->enabled = FALSE;
	
	// Forward all searches in this backend
	GList *iter = backend->searches;
	while (iter)
	{
		if (backend->cancel)
			backend->cancel(art,  (ArtSearch *) iter->data, backend->cancel);
		art_forward_search(art, (ArtSearch *) iter->data);

		iter = iter->next;
	}

	// Delete backend
	art->backends = g_list_remove(art->backends, backend);
	g_free(backend);
}

// --
// Once we have some real code we need to handle searches and cancelations from
// the app
// --
ArtSearch*
art_search(Art *art, LomoStream *stream, ArtFunc success, ArtFunc fail, gpointer data)
{
	ArtSearch *search = art_search_new(stream, success, fail, data);
	search->backend_link = art->backends;

	art->searches = g_list_append(art->searches, search);
	if (search->backend_link && search->backend_link->data)
	{
		ArtBackend *backend = (ArtBackend *) search->backend_link->data;
		backend->searches = g_list_append(backend->searches, search);
		art_backend_search(backend, search);
	}
	else
	{
		art_report_failure(art, search);
	}
	return search;
}

void
art_cancel(Art *art, ArtSearch *search)
{
	// Must be linked and running, otherwise this call has no sense.
	g_return_if_fail(search->backend_link && search->backend_link->data);
	g_return_if_fail(search->running);

	ArtBackend *backend = search->backend_link->data;

#ifdef ART_AHEAD
	g_return_if_fail(backend);
	art_backend_cancel(backend, search);
#else
	// Just call cancel hook if exists and delete
	gel_warn("Cancel in backend %s", backend->name);
	if (backend->cancel)
		backend->cancel(art, search, backend->data);
#endif

	backend->searches = g_list_remove(backend->searches, search);
	art->searches     = g_list_remove(art->searches, search);

	g_free(search);
}

void
art_fail(Art *art, ArtSearch *search)
{
	// Must be unlinked, this function only is reacheable if all backends
	// failed, must be stopped for the same reason.
	g_return_if_fail(!search->backend_link);
	g_return_if_fail(!search->running);

	if (search->fail)
		search->fail(art, search, search->data);
	
	// Search is not more useful, this is an exit point, wipe it.
	art->searches = g_list_remove(art->searches, search);
	art_search_destroy(search);
}

// --
// NG
// --
ArtBackend *
art_backend_new(Art *art, gchar *name, ArtFunc search, ArtFunc cancel, gpointer data)
{
	ArtBackend *backend = g_new0(ArtBackend, 1);
	backend->name       = g_strdup(name);
	backend->search     = search;
	backend->cancel     = cancel;
	backend->data       = data;
	backend->enabled    = TRUE;
	backend->art        = art;
	return backend;
}

void
art_backend_destroy(ArtBackend *backend)
{
	// XXX Running searches?

	g_free(backend->name);
	g_free(backend);
}

void
art_backend_search(ArtBackend *backend, ArtSearch *search)
{
	g_return_if_fail(search->backend_link->data == backend);
	g_return_if_fail(!search->running);

	gel_warn("Run search on backend '%s'", backend->name);
	search->running = TRUE;
	if (backend->search)
		backend->search(backend->art, search, backend->data);
}

void
art_backend_cancel(ArtBackend *backend, ArtSearch *search)
{
	g_return_if_fail(search->backend_link->data == backend);
	g_return_if_fail(search->running);

	gel_warn("Run cancel on backend '%s'", backend->name);
	if (backend->cancel)
		backend->cancel(backend->art, search, backend->data);
	search->running = FALSE;
}

ArtSearch*
art_search_new(LomoStream *stream, ArtFunc success, ArtFunc fail, gpointer data)
{
	ArtSearch *search = g_new0(ArtSearch, 1);
	search->stream  = stream;
	search->success = success;
	search->fail    = fail;
	search->data    = data;
	return search;
}

void
art_search_destroy(ArtSearch *search)
{
	g_return_if_fail(!search->running);
	g_return_if_fail(search->backend_link == NULL);
	g_free(search);
}
// --
// / NG
// --

// --
// Backends need to report Art object his success or failure, these two
// functions do the work
// --
void
art_report_success(Art *art, ArtSearch *search, gpointer result)
{
	// Search must be running, linked and result valid
	g_return_if_fail(search->running);
	g_return_if_fail(search->backend_link);
	g_return_if_fail(result);

	// Stop, unlink, merge result
	ArtBackend *backend = (ArtBackend *) search->backend_link->data;
	backend->searches = g_list_remove(backend->searches, search);
	search->backend_link = NULL;
	search->running = FALSE;

	search->result = result;

	if (search->success)
		search->success(art, search, search->data);

	art->searches = g_list_remove(art->searches, search);

	art_search_destroy(search);
}

void
art_report_failure(Art *art, ArtSearch *search)
{
	// Search must be running and linked
	g_return_if_fail(search->running);
	g_return_if_fail(search->backend_link);

	// Stop
	search->running = FALSE;

	// Forward
	art_forward_search(art, search);
}

// --
// More than one time searches can fail. This function moves failed searches
// over the backend list and runs them
// --
static void
art_forward_search(Art *art, ArtSearch *search)
{
	// Search must be stopped but linked
	g_return_if_fail(!search->running);
	g_return_if_fail(search->backend_link && search->backend_link->data);

	ArtBackend *old_backend = (ArtBackend *) search->backend_link->data;
	old_backend->searches = g_list_remove(old_backend->searches, search);

	GList *iter = search->backend_link->next;
	while (iter && !((ArtBackend *) iter->data)->enabled)
		iter = iter->next;
	
	if (iter == NULL)
	{
		gel_warn("No more backends availables.");
		// XXX: Make a function
		if (search->fail)
			search->fail(art, search, search->data);
		art->searches = g_list_remove(art->searches, search);
		g_free(search);
	}
	else
	{
		ArtBackend *new_backend = (ArtBackend *)iter->data;
		search->backend_link = iter;
		new_backend->searches = g_list_append(new_backend->searches, search);
		gel_warn("Search move from %s to %s", old_backend->name, new_backend->name);
		art_backend_search(new_backend, search);
	}
}

void
art_run_search(Art *art, ArtSearch *search)
{
	if (!search->backend_link || !search->backend_link->data)
		art_run_fail(art, search);
	else
	{
		ArtBackend *backend = (ArtBackend *) search->backend_link->data;
	
		gel_warn("Run backend %s", backend->name);
		search->running   = TRUE;
		backend->searches = g_list_append(backend->searches, search);
		backend->search(art, search, backend->data);
	}
}


void
art_run_fail(Art *art, ArtSearch *search)
{
	search->fail(art, search, search->data);
	art->searches = g_list_remove(art->searches, search);
	g_free(search);
}

// --
// ArtSearch access functions
// --
gpointer
art_search_get_result(ArtSearch *search)
{
	return search->result;
}

static gboolean
plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	plugin->data = art_new();
	gel_app_shared_set(app, "art", plugin->data);
	return TRUE;
}

static gboolean
plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	art_destroy((Art *) plugin->data);
	gel_app_shared_unregister(app, "art");
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin art_plugin = {
	EINA_PLUGIN_SERIAL,
	"art", PACKAGE_VERSION,
	N_("Art"), N_("New art framework"), NULL,
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	plugin_init, plugin_fini,
	NULL, NULL
};

