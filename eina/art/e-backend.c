#include "e-backend.h"

G_DEFINE_TYPE (EinaArtBackend, eina_art_backend, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART_BACKEND, EinaArtBackendPrivate))

typedef struct _EinaArtBackendPrivate EinaArtBackendPrivate;

struct _EinaArtBackendPrivate {
	gchar *name;
	EinaArtBackendFunc search, cancel;
	GDestroyNotify notify;
	gpointer data;

	GList *searches;
};

enum {
	FINISH,
	LAST_SIGNAL
};
guint signals[LAST_SIGNAL] = { 0 };

static void
eina_art_backend_dispose (GObject *object)
{
	EinaArtBackend *backend = EINA_ART_BACKEND(object);
	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);

	if (priv->searches)
	{
		g_warning("Leaking searches. Expect problems.");
		priv->searches = NULL;
	}

	if (priv->notify)
	{
		priv->notify(priv->data);
		priv->notify = NULL;
	}

	if (priv->name)
	{
		g_free(priv->name);
		priv->name = NULL;
	}

	G_OBJECT_CLASS (eina_art_backend_parent_class)->dispose (object);
}

static void
eina_art_backend_class_init (EinaArtBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaArtBackendPrivate));

	object_class->dispose = eina_art_backend_dispose;

	signals[FINISH] = g_signal_new("finish",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaArtBackendClass, finish),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
}

static void
eina_art_backend_init (EinaArtBackend *self)
{
	GET_PRIVATE(self)->searches = NULL;
}

EinaArtBackend*
eina_art_backend_new (gchar *name, EinaArtBackendFunc search, EinaArtBackendFunc cancel, GDestroyNotify notify, gpointer data)
{
	g_return_val_if_fail(name,   NULL);
	g_return_val_if_fail(search, NULL);

	EinaArtBackend *backend = g_object_new (EINA_TYPE_ART_BACKEND, NULL);
	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);
	priv->name = g_strdup(name);
	priv->search = search;
	priv->cancel = cancel;
	priv->notify = notify;
	priv->data   = data;

	return backend;
}

const gchar *
eina_art_backend_get_name(EinaArtBackend *backend)
{
	return (const gchar *) GET_PRIVATE(backend)->name;
}

void
eina_art_backend_run(EinaArtBackend *backend, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	GList *link = (GList *) eina_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	EinaArtBackend *search_backend = EINA_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	// Fire it.
	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);
	priv->search(backend, search, priv->data);
}

void
eina_art_backend_finish(EinaArtBackend *backend, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	GList *link = (GList *) eina_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	EinaArtBackend *search_backend = EINA_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	g_signal_emit(backend, signals[FINISH], 0, search, NULL);
}

