/*
 * eina/eina/eina-art-backend.c
 *
 * Copyright (C) 2004-2010 Eina
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
	gpointer            backend_data;

	// Searches in progress (uint > 0 if in idle, 0 in progress)
	GHashTable *dict;
};

typedef struct {
	EinaArtBackend *backend;
	EinaArtSearch *search;
} backend_run_t;

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
			EinaArtSearch *s = EINA_ART_SEARCH(l->data);
			eina_art_backend_cancel(backend, s);
			g_signal_emit(backend, signals[FINISH], 0, s);

			l = l->next;
		}
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
eina_art_backend_new (gchar *name, EinaArtBackendFunc search, EinaArtBackendFunc cancel, GDestroyNotify notify, gpointer backend_data)
{
	g_return_val_if_fail(name,   NULL);
	g_return_val_if_fail(search, NULL);

	EinaArtBackend *backend = g_object_new (EINA_TYPE_ART_BACKEND, NULL);
	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);
	priv->name = g_strdup(name);
	priv->search = search;
	priv->cancel = cancel;
	priv->notify = notify;
	priv->backend_data = backend_data;

	return backend;
}

const gchar *
eina_art_backend_get_name(EinaArtBackend *backend)
{
	return (const gchar *) GET_PRIVATE(backend)->name;
}

gboolean
eina_art_backend_run_real(backend_run_t *pack)
{
	g_return_val_if_fail(pack != NULL, FALSE);

	EinaArtBackend *backend = pack->backend;
	EinaArtSearch  *search  = pack->search;
	g_free(pack);

	g_return_val_if_fail(EINA_IS_ART_BACKEND(backend), FALSE);
	g_return_val_if_fail(EINA_IS_ART_SEARCH(search), FALSE);

	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);

	g_hash_table_insert(priv->dict, search, 0);
	priv->search(backend, search, priv->backend_data);

	return FALSE;
}

void
eina_art_backend_run(EinaArtBackend *backend, EinaArtSearch *search)
{
	// Checks
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	GList *link = (GList *) eina_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	EinaArtBackend *search_backend = EINA_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);

	g_return_if_fail(g_hash_table_lookup(priv->dict, search) == NULL);

	// Jobs
	backend_run_t *pack = g_new0(backend_run_t, 1);
	pack->backend = backend;
	pack->search  = search;

	g_hash_table_insert(priv->dict,
		search,
		GUINT_TO_POINTER(g_idle_add((GSourceFunc) eina_art_backend_run_real, pack)));
}

void
eina_art_backend_cancel(EinaArtBackend *backend, EinaArtSearch *search)
{
	// Some checks
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	GList *link = (GList *) eina_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	EinaArtBackend *search_backend = EINA_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);

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

void
eina_art_backend_finish(EinaArtBackend *backend, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	GList *link = (GList *) eina_art_search_get_bpointer(search);
	g_return_if_fail(link != NULL);

	EinaArtBackend *search_backend = EINA_ART_BACKEND(link->data);
	g_return_if_fail(search_backend == backend);

	EinaArtBackendPrivate *priv = GET_PRIVATE(backend);
	g_hash_table_remove(priv->dict, search);
	g_signal_emit(backend, signals[FINISH], 0, search, NULL);
}

