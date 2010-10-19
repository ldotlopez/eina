#include "ea.h"
#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <gel/gel.h>
#include <gdk/gdk.h>

G_DEFINE_TYPE (EinaArt, eina_art, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART, EinaArtPrivate))

typedef struct _EinaArtPrivate EinaArtPrivate;

#define ENABLE_XDG_CACHE 1
#define ENABLE_TIMEOUT   0
#define ENABLE_DEBUG     0

#if ENABLE_DEBUG
#define debug(...) g_warning(__VA_ARGS__)
#else
#define debug(...) ;
#endif

struct _EinaArtBackend {
	gchar *name;
	EinaArt     *art;
	EinaArtFunc  search, cancel;
	GList   *searches;
	gpointer data;
	gboolean enabled;
};

struct _EinaArtSearch {
	EinaArt *art;
	LomoStream *stream;
	EinaArtFunc callback;
	GdkPixbuf *result;
	gpointer data;
	GList *backend_link;
	gboolean running;
	guint timeout_id;
};

struct _EinaArtPrivate {
	GList *backends;
	GList *searches;
};

static EinaArtBackend*
eina_art_backend_new(EinaArt *art, gchar *name, EinaArtFunc search, EinaArtFunc cancel, gpointer data);
static void
eina_art_backend_destroy(EinaArtBackend *backend);
static void
eina_art_backend_search(EinaArtBackend *backend, EinaArtSearch *search);
static void
eina_art_backend_cancel(EinaArtBackend *backend, EinaArtSearch *search);
static void
eina_art_forward_search(EinaArt *art, EinaArtSearch *search);

static EinaArtSearch*
eina_art_search_new(EinaArt *art, LomoStream *stream, EinaArtFunc callback, gpointer data);
static void
eina_art_search_destroy(EinaArtSearch *search);
static void
eina_art_search_enable_timeout(EinaArtSearch *search);
static void
eina_art_search_disable_timeout(EinaArtSearch *search);

#if ENABLE_TIMEOUT
static gboolean
eina_art_search_timeout_cb(EinaArtSearch *search);
#endif

#if ENABLE_XDG_CACHE
static void
eina_art_backend_xdg_cache(EinaArt *art, EinaArtSearch *search, gpointer data);
static void
xdg_save(EinaArtSearch *search);
static gchar *
xdg_gen_unique(LomoStream *stream);
static gboolean
xdg_ensure_folder(void);
static char*
xdg_strip_characters (const char *original);
#endif

// -------------------------
// -------------------------
// EinaArt object implementation
// -------------------------
// -------------------------
static void
eina_art_dispose (GObject *object)
{
	EinaArtPrivate *priv = GET_PRIVATE(EINA_ART(object));

	GList *b = priv->backends;
	while (b)
	{
		EinaArtBackend *backend = (EinaArtBackend *) b->data;
		GList *s = backend->searches;
		while (s)
		{
			EinaArtSearch *search = (EinaArtSearch *) s->data;
			if (search->running)
				eina_art_backend_cancel(backend, search);
			eina_art_search_destroy(search);

			s = s->next;
		}
		eina_art_backend_destroy(backend);

		b = b->next;
	}

	gel_free_and_invalidate(priv->searches, NULL, g_list_free);
	gel_free_and_invalidate(priv->backends, NULL, g_list_free);

	G_OBJECT_CLASS (eina_art_parent_class)->dispose (object);
}

static void
eina_art_class_init (EinaArtClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaArtPrivate));

	object_class->dispose = eina_art_dispose;
}

static void
eina_art_init (EinaArt *self)
{
	EinaArtPrivate *priv = GET_PRIVATE(self);
	priv->searches = priv->backends = NULL;

#if ENABLE_XDG_CACHE
	eina_art_add_backend(self, "xdg-cache", eina_art_backend_xdg_cache, NULL, NULL);
#endif
}

EinaArt*
eina_art_new (void)
{
	return g_object_new (EINA_TYPE_ART, NULL);
}

// --
// At the beginning of the time we need to add some backends and do some real
// work
// --
EinaArtBackend *
eina_art_add_backend(EinaArt *art, gchar *name, EinaArtFunc search, EinaArtFunc cancel, gpointer data)
{
	g_return_val_if_fail(EINA_IS_ART(art), NULL);
	g_return_val_if_fail(name,   NULL);
	g_return_val_if_fail(search, NULL);

	EinaArtPrivate *priv = GET_PRIVATE(art);

	EinaArtBackend *backend = eina_art_backend_new(art, name, search, cancel, data);
	priv->backends = g_list_append(priv->backends, backend);
	return backend;
}

void
eina_art_remove_backend(EinaArt *art, EinaArtBackend *backend)
{
	g_return_if_fail(EINA_IS_ART(art));
	g_return_if_fail(backend);

	EinaArtPrivate *priv = GET_PRIVATE(art);

	// Disable
	backend->enabled = FALSE;
	
	// Forward all searches in this backend
	GList *iter = backend->searches;
	while (iter)
	{
		if (backend->cancel)
			backend->cancel(art,  (EinaArtSearch *) iter->data, backend->cancel);
		eina_art_forward_search(art, (EinaArtSearch *) iter->data);

		iter = iter->next;
	}

	// Delete backend
	priv->backends = g_list_remove(priv->backends, backend);
	g_free(backend);
}

const gchar*
eina_art_backend_get_name(EinaArtBackend *backend)
{
	return backend->name;
}

// --
// Once we have some real code we need to handle searches and cancelations from
// the app and failures from ourselves
// --
EinaArtSearch*
eina_art_search(EinaArt *art, LomoStream *stream, EinaArtFunc callback , gpointer data)
{
	g_return_val_if_fail(EINA_IS_ART(art), NULL);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), NULL);
	g_return_val_if_fail(callback, NULL);

	EinaArtPrivate *priv = GET_PRIVATE(art);

	EinaArtSearch *search = eina_art_search_new(art, stream, callback, data);
	search->backend_link = priv->backends;

	debug("Start search for %p", stream);
	priv->searches = g_list_append(priv->searches, search);
	if (search->backend_link && search->backend_link->data)
	{
		EinaArtBackend *backend = (EinaArtBackend *) search->backend_link->data;
		backend->searches = g_list_append(backend->searches, search);
		eina_art_backend_search(backend, search);
	}
	else
	{
		// Cannot call eina_art_report_failure directly, current state is far from
		// function prerequisites. Call fail hook by hand
		// eina_art_report_failure(art, search);
		debug("Calling fail hook directly");
		if (search->callback)
			search->callback(art, search, search->data);
		priv->searches = g_list_remove(priv->searches, search);
		eina_art_search_destroy(search);
		search = NULL;
	}
	return search;
}

void
eina_art_cancel(EinaArt *art, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART(art));
	g_return_if_fail(search);

	EinaArtPrivate *priv = GET_PRIVATE(art);

	// Search is already completed but still in schudele queue, do nothing
	if (search->backend_link == NULL)
		return;

	// Must be linked and running, otherwise this call has no sense.
	g_return_if_fail(g_list_find(priv->searches, search));
	g_return_if_fail(search->backend_link && search->backend_link->data);
	g_return_if_fail(search->running);

	// Run backend cancel func
	eina_art_search_disable_timeout(search);

	EinaArtBackend *backend = search->backend_link->data;
	eina_art_backend_cancel(backend, search);

	// Wipe search, its not useful anymore
	search->backend_link = NULL;
	backend->searches = g_list_remove(backend->searches, search);
	priv->searches     = g_list_remove(priv->searches, search);

	eina_art_search_destroy(search);
}

static gboolean
eina_art_fail_idle(EinaArtSearch *search)
{
	debug("Calling fail hook");
	if (search->callback)
		search->callback(search->art, search, search->data);
	
	// Search is not more useful, this is an exit point, wipe it.
	eina_art_search_destroy(search);

	return FALSE;
}

void
eina_art_fail(EinaArt *art, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART(art));
	g_return_if_fail(search);

	EinaArtPrivate *priv = GET_PRIVATE(art);

	// Must be unlinked, this function only is reacheable if all backends
	// failed, must be stopped for the same reason.
	g_return_if_fail(search->backend_link == NULL);
	g_return_if_fail(search->running ==  FALSE);

	priv->searches = g_list_remove(priv->searches, search);

	debug("Scheduling fail");
	g_idle_add((GSourceFunc) eina_art_fail_idle, search);
}

// --
// We also need to move search from backend to backend
// --
static void
eina_art_forward_search(EinaArt *art, EinaArtSearch *search)
{
	// Search must be stopped but linked
	g_return_if_fail(!search->running);
	g_return_if_fail(search->backend_link && search->backend_link->data);

	EinaArtBackend *old_backend = (EinaArtBackend *) search->backend_link->data;
	old_backend->searches = g_list_remove(old_backend->searches, search);

	GList *iter = search->backend_link->next;
	while (iter && !((EinaArtBackend *) iter->data)->enabled)
		iter = iter->next;
	
	if (iter == NULL)
	{
		debug("No more backends availables.");
		search->backend_link = NULL;
		eina_art_fail(art, search);
	}
	else
	{
		EinaArtBackend *new_backend = (EinaArtBackend *)iter->data;
		search->backend_link = iter;
		new_backend->searches = g_list_append(new_backend->searches, search);
		debug("Search move from %s to %s", old_backend->name, new_backend->name);
		eina_art_backend_search(new_backend, search);
	}
}

// --
// Backends need to report EinaArt object his success or failure, these two
// functions do the work
// --
static gboolean
eina_art_report_success_idle(EinaArtSearch *search)
{
	debug("Calling success hook");

#if ENABLE_XDG_CACHE
	xdg_save(search);
#endif

	if (search->callback)
		 search->callback(search->art, search, search->data);
	GET_PRIVATE(search->art)->searches = g_list_remove(GET_PRIVATE(search->art)->searches, search);

	eina_art_search_destroy(search);
	return FALSE;
}

void
eina_art_report_success(EinaArt *art, EinaArtSearch *search, GdkPixbuf *result)
{
	// Search must be running, linked and result valid
	g_return_if_fail(search->running);
	g_return_if_fail(search->backend_link);
	g_return_if_fail(result);

	eina_art_search_disable_timeout(search);

	// Stop, unlink, merge result
	EinaArtBackend *backend = (EinaArtBackend *) search->backend_link->data;
	backend->searches = g_list_remove(backend->searches, search);
	search->backend_link = NULL;
	search->running = FALSE;

	debug("Schudeling success");
	search->result = result;
	g_idle_add((GSourceFunc) eina_art_report_success_idle, search);
}

void
eina_art_report_failure(EinaArt *art, EinaArtSearch *search)
{
	// Search must be running and linked
	g_return_if_fail(search->running);
	g_return_if_fail(search->backend_link);

	// Stop
	search->running = FALSE;
	eina_art_search_disable_timeout(search);

	// Forward
	eina_art_forward_search(art, search);
}

// -------------------------
// -------------------------
// EinaArtBackend object implementation
// -------------------------
// -------------------------
static EinaArtBackend*
eina_art_backend_new(EinaArt *art, gchar *name, EinaArtFunc search, EinaArtFunc cancel, gpointer data)
{
	EinaArtBackend *backend = g_new0(EinaArtBackend, 1);
	backend->name       = g_strdup(name);
	backend->search     = search;
	backend->cancel     = cancel;
	backend->data       = data;
	backend->enabled    = TRUE;
	backend->art        = art;
	return backend;
}

static void
eina_art_backend_destroy(EinaArtBackend *backend)
{
	GList *iter = backend->searches;

	while (iter)
	{
		eina_art_backend_cancel(backend, (EinaArtSearch *) iter->data);
		eina_art_forward_search(backend->art, (EinaArtSearch *) iter->data);
		iter = iter->next;
	}

	g_free(backend->name);
	g_free(backend);
}

static void
eina_art_backend_search(EinaArtBackend *backend, EinaArtSearch *search)
{
	g_return_if_fail(search->backend_link->data == backend);
	g_return_if_fail(!search->running);

	debug("Run search on backend '%s'", backend->name);
	search->running = TRUE;
	if (backend->search)
	{
		eina_art_search_enable_timeout(search);
		backend->search(backend->art, search, backend->data);
	}
}

static void
eina_art_backend_cancel(EinaArtBackend *backend, EinaArtSearch *search)
{
	g_return_if_fail(search->backend_link->data == backend);
	g_return_if_fail(search->running);

	debug("Run cancel on backend '%s'", backend->name);
	if (backend->cancel)
		backend->cancel(backend->art, search, backend->data);
	search->running = FALSE;
}

// -------------------------
// -------------------------
// EinaArtBackend object implementation
// -------------------------
// -------------------------
static EinaArtSearch*
eina_art_search_new(EinaArt *art, LomoStream *stream, EinaArtFunc callback, gpointer data)
{
	EinaArtSearch *search = g_new0(EinaArtSearch, 1);
	search->art      = art;
	search->stream   = stream;
	search->callback = callback;
	search->data     = data;
	return search;
}

static void
eina_art_search_destroy(EinaArtSearch *search)
{
	g_return_if_fail(!search->running);
	g_return_if_fail(search->backend_link == NULL);
	if (search->timeout_id)
		eina_art_search_disable_timeout(search);
	g_free(search);
}

static void
eina_art_search_enable_timeout(EinaArtSearch *search)
{
#if ENABLE_EINA_ART_TIMEOUT
	g_return_if_fail(search->timeout_id == 0);
	debug("Enabling timeout on %p", search);
#if GLIB_CHECK_VERSION(2,14,0)
	search->timeout_id = g_timeout_add_seconds(10, (GSourceFunc) eina_art_search_timeout_cb, search);
#else
	search->timeout_id = g_timeout_add(10 * 1000, (GSourceFunc) eina_art_search_timeout_cb, search);
#endif
#endif
}

static void
eina_art_search_disable_timeout(EinaArtSearch *search)
{
#if ENABLE_EINA_ART_TIMEOUT
	g_return_if_fail(search->timeout_id);
	debug("Disabled timeout on %p", search);
	g_source_remove(search->timeout_id);
	search->timeout_id = 0;
#endif
}

#if ENABLE_EINA_ART_TIMEOUT
static gboolean
eina_art_search_timeout_cb(EinaArtSearch *search)
{
	debug("Timout fired!");
	// Call cancel on backend
	if (
		search->backend_link &&
		search->backend_link->data)
	{
		eina_art_backend_cancel(search->backend_link->data, search);
	}

	search->timeout_id = 0;

	// Move forward
	eina_art_forward_search(search->art, search);

	return FALSE;
}
#endif

LomoStream *
eina_art_search_get_stream(EinaArtSearch *search)
{
	return search->stream;
}

gpointer
eina_art_search_get_result(EinaArtSearch *search)
{
	return search->result;
}

gpointer
eina_art_search_get_data(EinaArtSearch *search)
{
	return search->data;
}

#if ENABLE_XDG_CACHE
// --
// Pseudo backend
// --
static gchar *
xdg_gen_unique(LomoStream *stream)
{
	// First round, with fallbacks
	const gchar *artist = (const gchar *) lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	const gchar *album =  (const gchar *) lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	if (!artist)
		artist = " ";
	if (!album)
		album = " ";

	// Normalize round
	gchar *a_norm, *b_norm;
	if (g_utf8_validate(artist, -1 , NULL))
		a_norm = g_utf8_normalize(artist, -1, G_NORMALIZE_NFKD);
	else
		a_norm = g_strdup(" ");

	if (g_utf8_validate(album, -1 , NULL))
		b_norm = g_utf8_normalize(album, -1, G_NORMALIZE_NFKD);
	else
		b_norm = g_strdup(" ");

	// Lowercase round
	gchar *a_lc = g_ascii_strdown(a_norm, -1);
	gchar *b_lc = g_ascii_strdown(b_norm, -1);
	g_free(a_norm);
	g_free(b_norm);

	// Strip round
	gchar *a_strip = xdg_strip_characters(a_lc);
	gchar *b_strip = xdg_strip_characters(b_lc);
	g_free(a_lc);
	g_free(b_lc);

	// Generate unique
	gchar *a_md5 = g_compute_checksum_for_string(G_CHECKSUM_MD5, a_strip, strlen(a_strip));
	gchar *b_md5 = g_compute_checksum_for_string(G_CHECKSUM_MD5, b_strip, strlen(b_strip));
	g_free(a_strip);
	g_free(b_strip);

	gchar *uniq = g_strconcat(g_get_user_cache_dir(), G_DIR_SEPARATOR_S, "media-art", G_DIR_SEPARATOR_S,  "album-", a_md5, "-", b_md5, ".jpeg", NULL);
	g_free(a_md5);
	g_free(b_md5);

	return uniq;	
}

static gboolean
xdg_ensure_folder(void)
{
	gchar *path = g_strconcat(g_get_user_cache_dir(), G_DIR_SEPARATOR_S, "media-art", G_DIR_SEPARATOR_S, NULL);
	gboolean ret = (g_mkdir_with_parents(path, 0755) == 0);
	g_free(path);
	return ret;
}

static char*
xdg_strip_characters (const char *original)
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

static void
xdg_save(EinaArtSearch *search)
{
	GdkPixbuf *pb = eina_art_search_get_result(search);
	g_return_if_fail(pb && GDK_IS_PIXBUF(pb));

	LomoStream *stream = eina_art_search_get_stream(search);
	gchar *uniq = xdg_gen_unique(stream);
	g_return_if_fail(uniq);

	debug("Saving cover for %s-%s to %s",
		(const gchar *) lomo_stream_get_tag(stream, LOMO_TAG_EINA_ARTIST),
		(const gchar *) lomo_stream_get_tag(stream, LOMO_TAG_ALBUM),
		uniq);

	if (g_file_test(uniq, G_FILE_TEST_EXISTS))
	{
		debug("Already cached, skipping");
		g_free(uniq);
		return;
	}
	
	GError *err = NULL;
	if (!gdk_pixbuf_save(pb, uniq, "jpeg", &err, "quality", "100", NULL))
	{
		debug("Cannot save");
		g_unlink(uniq);
		g_free(uniq);
		return;
	}

	debug("Saved");
	g_free(uniq);
}

static void
eina_art_backend_xdg_cache(EinaArt *art, EinaArtSearch *search, gpointer data)
{
	LomoStream *stream = eina_art_search_get_stream(search);
	const gchar *a = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	const gchar *b = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);

	if (!a || !b || !xdg_ensure_folder())
	{
		debug("Needed artist(%p) and album(%p) tags", a, b);
		eina_art_report_failure(art, search);
		return;
	}

	gchar *uniq = xdg_gen_unique(eina_art_search_get_stream(search));
	debug("Try for %s %s: %s", a, b, uniq);
	GdkPixbuf *pb = gdk_pixbuf_new_from_file(uniq, NULL);
	g_free(uniq);
	if (pb)
	{
		debug("XDG Cache successful");
		eina_art_report_success(art, search, pb);
	}
	else
		eina_art_report_failure(art, search);
}
#endif

