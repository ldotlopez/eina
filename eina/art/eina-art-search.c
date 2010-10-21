#define GEL_DOMAIN "EinaArtSearch"
#include "eina-art-search.h"
#include <glib/gi18n.h>
#include <gel/gel.h>

G_DEFINE_TYPE (EinaArtSearch, eina_art_search, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART_SEARCH, EinaArtSearchPrivate))

#define ENABLE_DEBUG 1
#if ENABLE_DEBUG
#define debug(...) gel_warn(__VA_ARGS__)
#else
#define debug(...) ;
#endif

typedef struct _EinaArtSearchPrivate EinaArtSearchPrivate;

struct _EinaArtSearchPrivate {
	GObject               *domain;
	LomoStream            *stream;
	EinaArtSearchCallback  callback;
	gpointer               callback_data;

	gchar *stringify;

	gpointer bpointer;

	gpointer result;
};

static void eina_art_search_weak_notify_cb(EinaArtSearch *search, GObject *old)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	g_warning(N_("Referenced %s from search '%s' was destroyed."),
		(old == priv->domain) ? "domain" : "stream",
		eina_art_search_stringify(search));
}

static void
eina_art_search_dispose (GObject *object)
{
	EinaArtSearch *search = EINA_ART_SEARCH(object);
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (priv->bpointer)
	{
		g_warning(N_("Search '%s' is running at dispose phase, calling callback and expect problems."), eina_art_search_stringify(search));
		priv->bpointer = NULL;
	}

	if (priv->callback)
	{
		eina_art_search_run_callback(search);
		priv->callback = NULL;
	}

	if (priv->domain)
	{
		g_object_weak_unref((GObject *) priv->domain, (GWeakNotify) eina_art_search_weak_notify_cb, search);
		g_object_unref(priv->domain);
		priv->domain = NULL;
	}

	if (priv->stream)
	{
		g_object_weak_unref((GObject *) priv->stream, (GWeakNotify) eina_art_search_weak_notify_cb, search);
		g_object_unref(priv->stream);
		priv->stream = NULL;
	}

	if (priv->stringify)
	{
		g_free(priv->stringify);
		priv->stringify = NULL;
	}

	G_OBJECT_CLASS (eina_art_search_parent_class)->dispose (object);
}

static void
eina_art_search_class_init (EinaArtSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaArtSearchPrivate));

	object_class->dispose = eina_art_search_dispose;
}

static void
eina_art_search_init (EinaArtSearch *self)
{
}

EinaArtSearch*
eina_art_search_new (GObject *domain, LomoStream *stream, EinaArtSearchCallback callback, gpointer callback_data)
{
	g_return_val_if_fail(G_IS_OBJECT(domain), NULL);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), NULL);
	g_return_val_if_fail(callback, NULL);

	EinaArtSearch *search = g_object_new (EINA_TYPE_ART_SEARCH, NULL);
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	priv->domain = g_object_ref(domain);
	priv->stream = g_object_ref(stream);
	priv->callback = callback;
	priv->callback_data = callback_data;

	g_object_weak_ref((GObject *) priv->stream, (GWeakNotify) eina_art_search_weak_notify_cb, search);
	g_object_weak_ref((GObject *) priv->domain, (GWeakNotify) eina_art_search_weak_notify_cb, search);

	return search;
}

GObject *
eina_art_search_get_domain(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->domain;
}

LomoStream *
eina_art_search_get_stream(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->stream;
}

void
eina_art_search_set_bpointer(EinaArtSearch *search, gpointer bpointer)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (bpointer && priv->bpointer)
	{
		g_warning(N_("Trying to set a bpointer while search '%s' already has a bpointer, this is a bug. You should set bpointer to NULL before trying to do this."),
			eina_art_search_stringify(search));
		g_return_if_fail(priv->bpointer == NULL);
	}

	priv->bpointer = bpointer;
}

gpointer
eina_art_search_get_bpointer(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->bpointer;
}

void
eina_art_search_set_result(EinaArtSearch *search, gpointer result)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (result && priv->result)
	{
		g_warning(N_("Trying to set a result in search '%s' while it already has one, this is a bug and will be ignored"),
			eina_art_search_stringify(search));
		g_return_if_fail(priv->result == NULL);
	}

	priv->result = result;
}

gpointer
eina_art_search_get_result(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->result;
}


const gchar*
eina_art_search_stringify(EinaArtSearch *search)
{
	g_return_val_if_fail(EINA_IS_ART_SEARCH(search), NULL);
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (!priv->stringify)
	{
		gchar *unescape = g_uri_unescape_string(lomo_stream_get_tag(priv->stream, LOMO_TAG_URI), NULL);
		priv->stringify = g_path_get_basename(unescape);
		g_free(unescape);
		g_return_val_if_fail(priv->stringify, NULL);
	}
	return priv->stringify;
}

void
eina_art_search_run_callback(EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	g_return_if_fail(priv->bpointer == NULL);

	if (!priv->callback)
	{
		g_warning("%s has been called multiple times on %s, ignoring.",
			__FUNCTION__,
			eina_art_search_stringify(search));
		return;
	}

	debug("Run callback for search %s", eina_art_search_stringify(search));
	priv->callback(search, priv->callback_data);
	priv->callback = NULL;
}

