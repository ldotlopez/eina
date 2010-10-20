#define GEL_DOMAIN "EinaArtSearch"
#include "eina-art-search.h"
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
	LomoStream *stream;
	EinaArtSearchCallback callback;
	gpointer data;

	gchar *stringify;

	GObject *domain;
	gpointer bpointer;

	gpointer result;
};

static void
eina_art_search_dispose (GObject *object)
{
	EinaArtSearch *search = EINA_ART_SEARCH(object);
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (priv->callback)
	{
		eina_art_search_run_callback(search);
		priv->callback = NULL;
	}

	if (priv->stream)
	{
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
eina_art_search_new (LomoStream *stream, EinaArtSearchCallback callback, gpointer data)
{
	g_return_val_if_fail(LOMO_IS_STREAM(stream), NULL);
	g_return_val_if_fail(callback, NULL);

	EinaArtSearch *search = g_object_new (EINA_TYPE_ART_SEARCH, NULL);
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	priv->stream = g_object_ref(stream);
	priv->callback = callback;
	priv->data = data;

	return search;
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

	if (bpointer)
		g_return_if_fail(priv->bpointer == NULL);

	GET_PRIVATE(search)->bpointer = bpointer;
}

gpointer
eina_art_search_get_bpointer(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->bpointer;
}

void
eina_art_search_set_domain(EinaArtSearch *search, GObject *domain)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (domain)
		g_return_if_fail(priv->domain == NULL);

	// FIXME: Add weak ref here
	GET_PRIVATE(search)->domain = domain;
}

GObject *
eina_art_search_get_domain(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->domain;
}

void
eina_art_search_set_result(EinaArtSearch *search, gpointer result)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (result)
		g_return_if_fail(priv->result == NULL);

	// FIXME: Add weak ref here
	GET_PRIVATE(search)->result = result;
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
	priv->callback(search, priv->data);
	priv->callback = NULL;
}

