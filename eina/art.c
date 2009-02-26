/*
 * eina/art.c
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

#define GEL_DOMAIN "Eina::Art"
#include "eina/art.h"
#include <eina/eina-plugin.h>
#include <config.h>

#define ENABLE_ART_XDG_CACHE 0
#define ENABLE_ART_TIMEOUT   0
#define ENABLE_ART_DEBUG     0

#if ENABLE_ART_DEBUG
#define art_debug(...) gel_warn(__VA_ARGS__)
#else
#define art_debug(...) ;
#endif

struct _Art {
	GList *backends;
	GList *searches;
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
	guint timeout_id;
};

static ArtBackend*
art_backend_new(Art *art, gchar *name, ArtFunc search, ArtFunc cancel, gpointer data);
static void
art_backend_destroy(ArtBackend *backend);
static void
art_backend_search(ArtBackend *backend, ArtSearch *search);
static void
art_backend_cancel(ArtBackend *backend, ArtSearch *search);
static void
art_forward_search(Art *art, ArtSearch *search);

static ArtSearch*
art_search_new(Art *art, LomoStream *stream, ArtFunc success, ArtFunc fail, gpointer data);
static void
art_search_destroy(ArtSearch *search);
static void
art_search_enable_timeout(ArtSearch *search);
static void
art_search_disable_timeout(ArtSearch *search);

#if ENABLE_ART_TIMEOUT
static gboolean
art_search_timeout_cb(ArtSearch *search);
#endif

#if ENABLE_ART_XDG_CACHE
void
art_backend_xdg_cache(Art *art, ArtSearch *search, gpointer data);
#endif

// -------------------------
// -------------------------
// Art object implementation
// -------------------------
// -------------------------

Art*
art_new(void)
{
	Art *art = g_new0(Art, 1);
	art->searches = art->backends = NULL;

#if ENABLE_ART_XDG_CACHE
	art_add_backend(art, "xdg-cache", art_backend_xdg_cache, NULL, NULL);
#endif
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

const gchar*
art_backend_get_name(ArtBackend *backend)
{
	return backend->name;
}

// --
// Once we have some real code we need to handle searches and cancelations from
// the app and failures from ourselves
// --
ArtSearch*
art_search(Art *art, LomoStream *stream, ArtFunc success, ArtFunc fail, gpointer data)
{
	ArtSearch *search = art_search_new(art, stream, success, fail, data);
	search->backend_link = art->backends;

	art_debug("Start search for %p", stream);
	art->searches = g_list_append(art->searches, search);
	if (search->backend_link && search->backend_link->data)
	{
		ArtBackend *backend = (ArtBackend *) search->backend_link->data;
		backend->searches = g_list_append(backend->searches, search);
		art_backend_search(backend, search);
	}
	else
	{
		// Cannot call art_report_failure directly, current state is far from
		// function prerequisites. Call fail hook by hand
		// art_report_failure(art, search);
		art_debug("Calling fail hook directly");
		if (search->fail)
			search->fail(art, search, search->data);
		art->searches = g_list_remove(art->searches, search);
		art_search_destroy(search);
		search = NULL;
	}
	return search;
}

void
art_cancel(Art *art, ArtSearch *search)
{
	// Search is already completed but still in schudele queue, do nothing
	if (search->backend_link == NULL)
		return;

	// Must be linked and running, otherwise this call has no sense.
	g_return_if_fail(g_list_find(art->searches, search));
	g_return_if_fail(search->backend_link && search->backend_link->data);
	g_return_if_fail(search->running);

	// Run backend cancel func
	art_search_disable_timeout(search);

	ArtBackend *backend = search->backend_link->data;
	art_backend_cancel(backend, search);

	// Wipe search, its not useful anymore
	search->backend_link = NULL;
	backend->searches = g_list_remove(backend->searches, search);
	art->searches     = g_list_remove(art->searches, search);

	art_search_destroy(search);
}

static gboolean
art_fail_idle(ArtSearch *search)
{
	art_debug("Calling fail hook");
	if (search->fail)
		search->fail(search->art, search, search->data);
	
	// Search is not more useful, this is an exit point, wipe it.
	art_search_destroy(search);

	return FALSE;
}

void
art_fail(Art *art, ArtSearch *search)
{
	// Must be unlinked, this function only is reacheable if all backends
	// failed, must be stopped for the same reason.
	g_return_if_fail(search->backend_link == NULL);
	g_return_if_fail(search->running ==  FALSE);

	art->searches = g_list_remove(art->searches, search);

	art_debug("Scheduling fail");
	g_idle_add((GSourceFunc) art_fail_idle, search);
}

// --
// We also need to move search from backend to backend
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
		art_debug("No more backends availables.");
		search->backend_link = NULL;
		art_fail(art, search);
	}
	else
	{
		ArtBackend *new_backend = (ArtBackend *)iter->data;
		search->backend_link = iter;
		new_backend->searches = g_list_append(new_backend->searches, search);
		art_debug("Search move from %s to %s", old_backend->name, new_backend->name);
		art_backend_search(new_backend, search);
	}
}

// --
// Backends need to report Art object his success or failure, these two
// functions do the work
// --
static gboolean
art_report_success_idle(ArtSearch *search)
{
	art_debug("Calling success hook");
	if (search->success)
		 search->success(search->art, search, search->data);
	search->art->searches = g_list_remove(search->art->searches, search);
	art_search_destroy(search);
	return FALSE;
}

void
art_report_success(Art *art, ArtSearch *search, gpointer result)
{
	// Search must be running, linked and result valid
	g_return_if_fail(search->running);
	g_return_if_fail(search->backend_link);
	g_return_if_fail(result);

	art_search_disable_timeout(search);

	// Stop, unlink, merge result
	ArtBackend *backend = (ArtBackend *) search->backend_link->data;
	backend->searches = g_list_remove(backend->searches, search);
	search->backend_link = NULL;
	search->running = FALSE;

	art_debug("Schudeling success");
	search->result = result;
	g_idle_add((GSourceFunc) art_report_success_idle, search);
}

void
art_report_failure(Art *art, ArtSearch *search)
{
	// Search must be running and linked
	g_return_if_fail(search->running);
	g_return_if_fail(search->backend_link);

	// Stop
	search->running = FALSE;
	art_search_disable_timeout(search);

	// Forward
	art_forward_search(art, search);
}

// -------------------------
// -------------------------
// ArtBackend object implementation
// -------------------------
// -------------------------
static ArtBackend*
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

static void
art_backend_destroy(ArtBackend *backend)
{
	GList *iter = backend->searches;

	while (iter)
	{
		art_backend_cancel(backend, (ArtSearch *) iter->data);
		art_forward_search(backend->art, (ArtSearch *) iter->data);
		iter = iter->next;
	}

	g_free(backend->name);
	g_free(backend);
}

static void
art_backend_search(ArtBackend *backend, ArtSearch *search)
{
	g_return_if_fail(search->backend_link->data == backend);
	g_return_if_fail(!search->running);

	art_debug("Run search on backend '%s'", backend->name);
	search->running = TRUE;
	if (backend->search)
	{
		art_search_enable_timeout(search);
		backend->search(backend->art, search, backend->data);
	}
}

static void
art_backend_cancel(ArtBackend *backend, ArtSearch *search)
{
	g_return_if_fail(search->backend_link->data == backend);
	g_return_if_fail(search->running);

	art_debug("Run cancel on backend '%s'", backend->name);
	if (backend->cancel)
		backend->cancel(backend->art, search, backend->data);
	search->running = FALSE;
}

// -------------------------
// -------------------------
// ArtBackend object implementation
// -------------------------
// -------------------------
static ArtSearch*
art_search_new(Art *art, LomoStream *stream, ArtFunc success, ArtFunc fail, gpointer data)
{
	ArtSearch *search = g_new0(ArtSearch, 1);
	search->art     = art;
	search->stream  = stream;
	search->success = success;
	search->fail    = fail;
	search->data    = data;
	return search;
}

static void
art_search_destroy(ArtSearch *search)
{
	g_return_if_fail(!search->running);
	g_return_if_fail(search->backend_link == NULL);
	if (search->timeout_id)
		art_search_disable_timeout(search);
	g_free(search);
}

static void
art_search_enable_timeout(ArtSearch *search)
{
#if ENABLE_ART_TIMEOUT
	g_return_if_fail(search->timeout_id == 0);
	art_debug("Enabling timeout on %p", search);
#if GLIB_CHECK_VERSION(2,14,0)
	search->timeout_id = g_timeout_add_seconds(10, (GSourceFunc) art_search_timeout_cb, search);
#else
	search->timeout_id = g_timeout_add(10 * 1000, (GSourceFunc) art_search_timeout_cb, search);
#endif
#endif
}

static void
art_search_disable_timeout(ArtSearch *search)
{
#if ENABLE_ART_TIMEOUT
	g_return_if_fail(search->timeout_id);
	art_debug("Disabled timeout on %p", search);
	g_source_remove(search->timeout_id);
	search->timeout_id = 0;
#endif
}

#if ENABLE_ART_TIMEOUT
static gboolean
art_search_timeout_cb(ArtSearch *search)
{
	art_debug("Timout fired!");
	// Call cancel on backend
	if (
		search->backend_link &&
		search->backend_link->data)
	{
		art_backend_cancel(search->backend_link->data, search);
	}

	search->timeout_id = 0;

	// Move forward
	art_forward_search(search->art, search);

	return FALSE;
}
#endif

LomoStream *
art_search_get_stream(ArtSearch *search)
{
	return search->stream;
}

gpointer
art_search_get_result(ArtSearch *search)
{
	return search->result;
}

gpointer
art_search_get_data(ArtSearch *search)
{
	return search->data;
}

// -------------------------
// -------------------------
// EinaPlugin interface
// -------------------------
// -------------------------
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
	N_("Art"), N_("Art framework"), NULL,
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	plugin_init, plugin_fini,
	NULL, NULL
};

#if ENABLE_ART_XDG_CACHE
// --
// Pseudo backend
// --
static char*
strip_characters (const char *original)
{
	const char *foo = "()[]<>{}_!@#$^&*+=|\\/\"'?~";
	int osize = strlen (original);
	char *retval = (char *) malloc (sizeof (char *) * osize);
	int i = 0, y = 0;

	while (i < osize) {

		/* Remove (anything) */

		if (original[i] == '(') {
			char *loc = strchr (original+i, ')');
			if (loc) {
				i = loc - original + 1;
				continue;
			}
		}

		/* Remove [anything] */

		if (original[i] == '[') {
			char *loc = strchr (original+i, ']');
			if (loc) {
				i = loc - original + 1;
				continue;
			}
		}

		/* Remove {anything} */

		if (original[i] == '{') {
			char *loc = strchr (original+i, '}');
			if (loc) {
				i = loc - original + 1;
				continue;
			}
		}

		/* Remove <anything> */

		if (original[i] == '<') {
			char *loc = strchr (original+i, '>');
			if (loc) {
				i = loc - original + 1;
				continue;
			}
		}

		/* Remove double whitespaces */

		if ((y > 0) &&
		    (original[i] == ' ' || original[i] == '\t') &&
		    (retval[y-1] == ' ' || retval[y-1] == '\t')) {
			i++;
			continue;
		}

		/* Remove strange characters */

		if (!strchr (foo, original[i])) {
			retval[y] = original[i]!='\t'?original[i]:' ';
			y++;
		}

		i++;
	}

	retval[y] = 0;

	return retval;
}

static gchar*
remove_unicode(gchar *input)
{
	g_return_val_if_fail(input != NULL, NULL);
	g_return_val_if_fail(g_utf8_validate(input, -1, NULL), NULL);

	// Lower case input
	gchar *lc = g_utf8_strdown(input);
	g_return_val_if_fail(lc, NULL);

	// Strip wide-characters
	gint i = 0;
	gchar *light = g_new0(gchar, g_utf8_strlen(lc, -1));
	gchar *p = lc;
	do
		if (g_ascii_isalnum(p[0]))
			light[i++] = p[0];
	} while (((p = g_utf8_find_next_char(p)) != NULL) && (p[0] != '\0'));
	g_free(lc);

	if (light[0] == '\0')
	{
		g_free(ret);
		return NULL;
	}

	// Strip trash 
	gchar *stripped = strip_characters(ret);
	g_free(ret);

	return stripped;
}

void
art_backend_xdg_cache(Art *art, ArtSearch *search, gpointer data)
{
	LomoStream *stream = art_search_get_stream(search);
	const gchar *a = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	const gchar *b = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);

	if (!a || !b)
	{
		gel_warn("Needed artist(%p) and album(%p) tags", a, b);
		art_report_failure(art, search);
		return;
	}

	gboolean a_utf8 = g_utf8_validate(a, -1, NULL);
	gboolean b_utf8 = g_utf8_validate(b, -1, NULL);
	if (!a_utf8 || !b_utf8)
	{
		gel_warn("Invalid artist or album tags (A_UTF8:%d, B_UTF8:%d)", a_utf8, b_utf8);
		art_report_failure(art, search);
		return;
	}

	// Lower case tags
	gchar *a_lc = g_utf8_strdown(a);
	gchar *b_lc = g_utf8_strdown(b);

	// Strip unicodes
	gchar *p = (gchar *) a;
	do
	{
		gel_warn("=> %c (0x%d)", p[0], p[0]);
	} while (( p = g_utf8_find_next_char(p, NULL)) != NULL && p[0] != '\0');
	art_report_failure(art, search);
}
#endif
