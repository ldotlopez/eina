/*
 * eina/art/eina-art-backend.c
 *
 * Copyright (C) 2004-2011 Eina
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

/**
 * SECTION:lomo-em-art-backend
 * @title:LomoEMArtBackend
 * @short_description: Pluggable backends for art discovery
 *
 * LomoEMArtBackend is the way to enable alternative art discovery (searching in
 * internet, hard-disk, tunesdb?). #LomoEMArtBackend handles #LomoEMArtSearch
 * objects not #LomoStream or URIs
 */

#include "lomo-em-art-backend.h"

G_DEFINE_TYPE (LomoEMArtBackend, lomo_em_art_backend, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_EM_ART_BACKEND, LomoEMArtBackendPrivate))

#define ENABLE_IDLE_RUN 1

typedef struct _LomoEMArtBackendPrivate LomoEMArtBackendPrivate;

struct _LomoEMArtBackendPrivate {
	gchar              *name;
	LomoEMArtBackendFunc  search, cancel;
	GDestroyNotify      notify;
	gpointer            backend_data;

	GHashTable *dict;
};

typedef struct {
	LomoEMArtBackend *backend;
	LomoEMArtSearch *search;
} backend_run_t;

enum {
	FINISH,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
lomo_em_art_backend_dispose (GObject *object)
{
	LomoEMArtBackend *backend = LOMO_EM_ART_BACKEND(object);
	LomoEMArtBackendPrivate *priv = GET_PRIVATE(backend);

	if (priv->notify)
	{
		priv->notify(priv->backend_data);
		priv->notify = NULL;
	}

	if (priv->dict)
	{
		GList *searches = g_hash_table_get_keys(priv->dict);
		GList *l = searches;
		while (l)
		{
			LomoEMArtSearch *s = LOMO_EM_ART_SEARCH(l->data);
			lomo_em_art_backend_cancel(backend, s);
			g_signal_emit(backend, signals[FINISH], 0, s);

			l = l->next;
		}
	}

	if (priv->name)
	{
		g_free(priv->name);
		priv->name = NULL;
	}

	G_OBJECT_CLASS (lomo_em_art_backend_parent_class)->dispose (object);
}

static void
lomo_em_art_backend_class_init (LomoEMArtBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoEMArtBackendPrivate));

	object_class->dispose = lomo_em_art_backend_dispose;

	/**
	 * LomoEMArtBackend::finish
	 * @backend: The #LomoEMArtBackend
	 * @search: The finished #LomoEMArtSearch
	 *
	 * Emitted after the @search has beed finalized. This signal is mean to be
	 * handled by #LomoEMArt not for direct use.
	 */
	signals[FINISH] = g_signal_new("finish",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (LomoEMArtBackendClass, finish),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
}

static void
lomo_em_art_backend_init (LomoEMArtBackend *self)
{
	LomoEMArtBackendPrivate *priv = GET_PRIVATE(self);
	priv->dict = g_hash_table_new(g_direct_hash, g_direct_equal);
}

/**
 * lomo_em_art_backend_new:
 * @name: A (unique) name for the backend
 * @search: Function to search art data
 * @cancel: Function to cancel a running search
 * @notify: Destroy notify function to free @backend_data
 * @backend_data: Specific backend data (like userdata)
 *
 * Returns: The backend
 */
LomoEMArtBackend*
lomo_em_art_backend_new (const gchar *name,
	LomoEMArtBackendFunc search, LomoEMArtBackendFunc cancel, GDestroyNotify notify, gpointer backend_data)
{
	g_return_val_if_fail(name,   NULL);
	g_return_val_if_fail(search, NULL);

	LomoEMArtBackend *backend = g_object_new (LOMO_TYPE_EM_ART_BACKEND, NULL);
	LomoEMArtBackendPrivate *priv = GET_PRIVATE(backend);
	priv->name = g_strdup(name);
	priv->search = search;
	priv->cancel = cancel;
	priv->notify = notify;
	priv->backend_data = backend_data;

	return backend;
}

/**
 * lomo_em_art_backend_get_name:
 * @backend: An #LomoEMArtBackend
 *
 * Gets the name of the backend
 *
 * Returns: The name
 */
const gchar *
lomo_em_art_backend_get_name(LomoEMArtBackend *backend)
{
	return (const gchar *) GET_PRIVATE(backend)->name;
}

gboolean
lomo_em_art_backend_run_real(backend_run_t *pack)
{
	g_return_val_if_fail(pack != NULL, FALSE);

	LomoEMArtBackend *backend = pack->backend;
	LomoEMArtSearch  *search  = pack->search;
	g_free(pack);

	g_return_val_if_fail(LOMO_IS_EM_ART_BACKEND(backend), FALSE);
	g_return_val_if_fail(LOMO_IS_EM_ART_SEARCH(search), FALSE);

	LomoEMArtBackendPrivate *priv = GET_PRIVATE(backend);

	g_hash_table_insert(priv->dict, search, 0);
	priv->search(backend, search, priv->backend_data);

	return FALSE;
}

/**
 * lomo_em_art_backend_run:
 * @backend: An #LomoEMArtBackend
 * @search: An #LomoEMArtSearch
 *
 * Runs @search in @backend. This will call search function from @backend
 */
void
lomo_em_art_backend_run(LomoEMArtBackend *backend, LomoEMArtSearch *search)
{
	// Checks
	g_return_if_fail(LOMO_IS_EM_ART_BACKEND(backend));
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));

	GList *link = (GList *) lomo_em_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	LomoEMArtBackend *search_backend = LOMO_EM_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	LomoEMArtBackendPrivate *priv = GET_PRIVATE(backend);

	g_return_if_fail(g_hash_table_lookup(priv->dict, search) == NULL);

	// Jobs
	backend_run_t *pack = g_new0(backend_run_t, 1);
	pack->backend = backend;
	pack->search  = search;

	g_hash_table_insert(priv->dict,
		search,
		GUINT_TO_POINTER(g_idle_add((GSourceFunc) lomo_em_art_backend_run_real, pack)));
}

/**
 * lomo_em_art_backend_cancel:
 * @backend: An #LomoEMArtBackend
 * @search: An #LomoEMArtSearch
 *
 * Cancels running @search in @backend. This will call cancel function from
 * @backend
 */
void
lomo_em_art_backend_cancel(LomoEMArtBackend *backend, LomoEMArtSearch *search)
{
	// Some checks
	g_return_if_fail(LOMO_IS_EM_ART_BACKEND(backend));
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));

	GList *link = (GList *) lomo_em_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	LomoEMArtBackend *search_backend = LOMO_EM_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	LomoEMArtBackendPrivate *priv = GET_PRIVATE(backend);

	gpointer pstate = g_hash_table_lookup(priv->dict, search);
	g_return_if_fail(pstate == NULL);

	// Real job
	guint state = GPOINTER_TO_UINT(pstate);
	if (state > 0)
		g_source_remove(state);
	else
	{
		if (priv->cancel)
			priv->cancel(backend, search, priv->backend_data);
	}
	g_hash_table_remove(priv->dict, search);
}

/**
 * lomo_em_art_backend_finish:
 * @backend: An #LomoEMArtBackend
 * @search: An #LomoEMArtBackend
 *
 * Finalizes the seach in @backend, this function had to be called after the
 * search function from @backend has do they job. If #LomoEMArtSearch has result
 * it will be passed to #LomoEMArt object to be dispached, otherwise the next
 * backend will try to find art data.
 */
void
lomo_em_art_backend_finish(LomoEMArtBackend *backend, LomoEMArtSearch *search)
{
	g_return_if_fail(LOMO_IS_EM_ART_BACKEND(backend));
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));

	GList *link = (GList *) lomo_em_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	LomoEMArtBackend *search_backend = LOMO_EM_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	LomoEMArtBackendPrivate *priv = GET_PRIVATE(backend);
	g_hash_table_remove(priv->dict, search);
	g_signal_emit(backend, signals[FINISH], 0, search, NULL);
}

