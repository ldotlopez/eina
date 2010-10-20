#include "eina-art-backend.h"

G_DEFINE_TYPE (EinaArtBackend, eina_art_backend, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART_BACKEND, EinaArtBackendPrivate))

#define ENABLE_IDLE_RUN 1

typedef struct _EinaArtBackendPrivate EinaArtBackendPrivate;

struct _EinaArtBackendPrivate {
	gchar              *name;
	EinaArtBackendFunc  search, cancel;
	GDestroyNotify      notify;
	gpointer            data;

	// Searches in progress
	GHashTable *dict;

	#if ENABLE_IDLE_RUN
	guint idle_id;
	#endif
};

#if ENABLE_IDLE_RUN
typedef struct {
	EinaArtBackend *backend;
	EinaArtSearch *search;
} BackendRunPack;
#endif

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

	if (priv->dict)
	{
		GList *searches = g_hash_table_get_keys(priv->dict);
		if (searches)
			g_warning("Leaking searches. Expect problems.");
		g_list_free(searches);
		g_hash_table_destroy(priv->dict);
		priv->dict = NULL;
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
	EinaArtBackendPrivate *priv = GET_PRIVATE(self);
	priv->dict = g_hash_table_new(g_direct_hash, g_direct_equal);
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

#if ENABLE_IDLE_RUN
gboolean
eina_art_backend_run_real(BackendRunPack *pack)
{
	g_return_val_if_fail(pack != NULL, FALSE);

	EinaArtBackend *backend = pack->backend;
	EinaArtSearch  *search  = pack->search;
	g_free(pack);

	g_return_val_if_fail(EINA_IS_ART_BACKEND(backend), FALSE);
	g_return_val_if_fail(EINA_IS_ART_SEARCH(search), FALSE);

	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);

	g_hash_table_remove(priv->dict, search);
	priv->search(backend, search, priv->data);

	return FALSE;
}
#endif

void
eina_art_backend_run(EinaArtBackend *backend, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);

	GList *link = (GList *) eina_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	EinaArtBackend *search_backend = EINA_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	g_return_if_fail(g_hash_table_lookup(priv->dict, search) == NULL);

	#if ENABLE_IDLE_RUN

	BackendRunPack *pack = g_new0(BackendRunPack, 1);
	pack->backend = backend;
	pack->search  = search;

	g_hash_table_insert(priv->dict,
		search,
		GUINT_TO_POINTER(g_idle_add((GSourceFunc) eina_art_backend_run_real, pack)));

	#else

	// Fire it.
	g_hash_table_insert(priv->dict,
		priv->search(backend, search, priv->data),
		(gpointer) TRUE);
	
	#endif
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

