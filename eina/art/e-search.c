#include "e-search.h"

G_DEFINE_TYPE (EinaArtSearch, eina_art_search, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART_SEARCH, EinaArtSearchPrivate))

typedef struct _EinaArtSearchPrivate EinaArtSearchPrivate;

struct _EinaArtSearchPrivate {
	LomoStream *stream;
	EinaArtSearchCallback callback;
	gpointer data;

	gpointer bpointer;
	GObject *owner;
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

void
eina_art_search_run_callback(EinaArtSearch *self)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(self));

	EinaArtSearchPrivate *priv = GET_PRIVATE(self);
	if (priv->callback)
		priv->callback(self, priv->data);
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
eina_art_search_set_owner(EinaArtSearch *search, GObject *owner)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (owner)
		g_return_if_fail(priv->owner == NULL);

	GET_PRIVATE(search)->owner = owner;
}

GObject*
eina_art_search_get_owner(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->owner;
}

