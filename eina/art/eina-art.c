/*
 * eina/eina/eina-art.c
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
#define GEL_DOMAIN "EinaArt"
#include "eina-art.h"
#include <gel/gel.h>

G_DEFINE_TYPE (EinaArt, eina_art, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART, EinaArtPrivate))

#if EINA_ART_DEBUG
#define debug(...) gel_warn(__VA_ARGS__)
#else
#define debug(...) ;
#endif

typedef struct _EinaArtPrivate EinaArtPrivate;

struct _EinaArtClassPrivate {
	GList *backends;
};

struct _EinaArtPrivate {
	GList *searches;
};

static void
backend_finish_cb(EinaArtBackend *backend, EinaArtSearch *search, EinaArtClass *art_class);

static void
eina_art_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_art_parent_class)->dispose (object);
}

static void
eina_art_class_init (EinaArtClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaArtPrivate));

	object_class->dispose = eina_art_dispose;

	klass->priv = g_new0(EinaArtClassPrivate, 1);
}

static void
eina_art_init (EinaArt *self)
{
}

EinaArt*
eina_art_new (void)
{
	return g_object_new (EINA_TYPE_ART, NULL);
}

EinaArtBackend*
eina_art_class_add_backend(EinaArtClass *art_class,
                           gchar *name,
                           EinaArtBackendFunc search, EinaArtBackendFunc cancel,
                           GDestroyNotify notify, gpointer data)
{
	g_return_val_if_fail(EINA_IS_ART_CLASS(art_class), NULL);

	EinaArtBackend *backend = eina_art_backend_new(name, search, cancel, notify, data);
	g_return_val_if_fail(backend != NULL, NULL);

	art_class->priv->backends = g_list_append(art_class->priv->backends, backend);
	g_signal_connect(backend, "finish", (GCallback) backend_finish_cb, art_class);

	return backend;
}

void
eina_art_class_remove_backend(EinaArtClass *art_class, EinaArtBackend *backend)
{
	g_return_if_fail(EINA_IS_ART_CLASS(art_class));
	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	
	GList *link = g_list_find(art_class->priv->backends, backend);
	g_return_if_fail(link);

	art_class->priv->backends = g_list_remove_link(art_class->priv->backends, link);
	g_object_unref(backend);
	g_list_free(link);
}

void
eina_art_class_try_next_backend(EinaArtClass *art_class, EinaArtSearch *search)
{
	GList *link = eina_art_search_get_bpointer(search);
	
	// Invalid search
	if (!link || !g_list_find(art_class->priv->backends, link->data))
	{
		g_warning("Search %p is not in our universe, reporting failure", search);
		eina_art_search_set_bpointer(search, NULL);
		eina_art_search_run_callback(search);
		g_object_unref(search);
		return;
	}

	// Jump to the next backend
	if (link->next && link->next->data)
	{
		debug("Moving search %s (%s -> %s)",
			eina_art_search_stringify(search),
			eina_art_backend_get_name(EINA_ART_BACKEND(link->data)),
			eina_art_backend_get_name(EINA_ART_BACKEND(link->next->data)));
		EinaArtBackend *backend = EINA_ART_BACKEND(link->next->data);
		eina_art_search_set_bpointer(search, NULL);
		eina_art_search_set_bpointer(search, g_list_next(link));
		eina_art_backend_run(backend, search);
		return;
	}

	// No more backends, report and remove
	debug("Search %s has failed, reporting to caller", eina_art_search_stringify(search));
	EinaArt *art = EINA_ART(eina_art_search_get_domain(search));
	EinaArtPrivate *priv = GET_PRIVATE(art);
	priv->searches = g_list_remove(priv->searches, search);

	eina_art_search_set_bpointer(search, NULL);
	eina_art_search_run_callback(search);
	g_object_unref(search);
}

EinaArtSearch*
eina_art_search(EinaArt *art, LomoStream *stream, EinaArtSearchCallback callback, gpointer data)
{
	g_return_val_if_fail(EINA_IS_ART(art), NULL);

	EinaArtSearch *search = eina_art_search_new((GObject *) art, stream, callback, data);
	g_return_val_if_fail(search != NULL, NULL);

	EinaArtPrivate *priv = GET_PRIVATE(art);
	priv->searches = g_list_append(priv->searches, search);

	GList *backends = EINA_ART_GET_CLASS(art)->priv->backends;
	
	if (!backends || !backends->data)
	{
		// Report failure
		eina_art_search_run_callback(search);
		priv->searches = g_list_remove(priv->searches, search);
		g_object_unref(search);
		return NULL;
	}

	// We have backends, run them
	eina_art_search_set_bpointer(search, backends);
	eina_art_backend_run(EINA_ART_BACKEND(backends->data), search);

	return search;
}

void
eina_art_cancel(EinaArt *art, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_ART(art));
	g_return_if_fail(EINA_IS_ART(search));

	g_return_if_fail(art == EINA_ART(eina_art_search_get_domain(search)));

	EinaArtClass   *art_class = EINA_ART_GET_CLASS(art);
	EinaArtPrivate *priv      = GET_PRIVATE(art);
	EinaArtBackend *backend   = EINA_ART_BACKEND(((GList *)eina_art_search_get_bpointer(search))->data);

	g_return_if_fail(EINA_IS_ART_BACKEND(backend));
	g_return_if_fail(g_list_find(art_class->priv->backends, backend) == NULL);

	eina_art_backend_cancel(backend, search);
	priv->searches = g_list_remove(priv->searches, search);
	g_object_unref(search);
}

static void
backend_finish_cb(EinaArtBackend *backend, EinaArtSearch *search, EinaArtClass *art_class)
{
	gpointer result = eina_art_search_get_result(search);

	// Got results
	if (result)
	{
		EinaArt *art = EINA_ART(eina_art_search_get_domain(search));
		EinaArtPrivate *priv = GET_PRIVATE(art);

		priv->searches = g_list_remove(priv->searches, search);
		eina_art_search_set_bpointer(search, NULL);

		debug("Backend '%s' successfull", eina_art_backend_get_name(backend));
		eina_art_search_run_callback(search);
		g_object_unref(search);
		return;
	}

	// No results
	eina_art_class_try_next_backend(art_class, search);
}
