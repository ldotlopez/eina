/*
 * eina/art/lomo-em-art.c
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
 * SECTION:lomo-em-art
 * @title: LomoEMArt
 * @short_description: Art data discovery object
 *
 * #LomoEMArt is the way to discover art metadata for #LomoStream. It provides a
 * pair of functions for start and cancel searches: lomo_em_art_search() and
 * lomo_em_art_cancel(). All instaces of #LomoEMArt share the same backends, if one
 * #LomoEMArtBackend is added all instances are able to use it.
 *
 * <example>
 * <title>Basic flow for search</title>
 * <programlisting>
 * LomoEMArtSearch *_search;
 *
 * void callback(LomoEMArtSearch *search) {
 *   const gchar *art_uri = lomo_em_art_search_get_result(search);
 *   [do something with art_uri]
 * 	 *_search = NULL;
 * }
 *
 * [your code]
 * *_search = lomo_em_art_search(art, stream, callback, data);
 * // Save search variable for eventual cancelation
 * [your code]
 * </programlisting>
 * </example>
 */

#include "lomo-em-art.h"

#define DEBUG 0
#define DEBUG_PREFIX "LomoEMArt"
#if DEBUG
#	define debug(...) g_debug(DEBUG_PREFIX" "__VA_ARGS__)
#else
#	define debug(...) ;
#endif

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_EM_ART, LomoEMArtPrivate))

G_DEFINE_TYPE (LomoEMArt, lomo_em_art, G_TYPE_OBJECT)

typedef struct _LomoEMArtPrivate LomoEMArtPrivate;
struct _LomoEMArtClassPrivate {
	GList *backends;
};

struct _LomoEMArtPrivate {
	GList *searches;
};

static void
backend_finish_cb(LomoEMArtBackend *backend, LomoEMArtSearch *search, LomoEMArtClass *art_class);

static void
lomo_em_art_dispose (GObject *object)
{
	G_OBJECT_CLASS (lomo_em_art_parent_class)->dispose (object);
}

static void
lomo_em_art_class_init (LomoEMArtClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoEMArtPrivate));

	object_class->dispose = lomo_em_art_dispose;

	klass->priv = g_new0(LomoEMArtClassPrivate, 1);
}

static void
lomo_em_art_init (LomoEMArt *self)
{
}

/**
 * lomo_em_art_new:
 *
 * Creates a new #LomoEMArt object
 *
 * Returns: The #LomoEMArt object
 */
LomoEMArt*
lomo_em_art_new (void)
{
	return g_object_new (LOMO_TYPE_EM_ART, NULL);
}

/**
 * lomo_em_art_class_add_backend:
 * @art_class: An #LomoEMArtClass
 * @name: An (unique) name for the backend
 * @search: Search function
 * @cancel: Cancel function
 * @notify: Notify function
 * @data: User data
 *
 * Adds a new backend to #LomoEMArtClass, see lomo_em_art_backend_new() for details.
 * Note that this function adds backend to the class instead of the instance.
 *
 * Returns: (transfer none): The newly created add added #LomoEMArtBackend
 */
LomoEMArtBackend*
lomo_em_art_class_add_backend(LomoEMArtClass *art_class,
	gchar *name, LomoEMArtBackendFunc search, LomoEMArtBackendFunc cancel, GDestroyNotify notify, gpointer data)
{
	g_return_val_if_fail(LOMO_IS_EM_ART_CLASS(art_class), NULL);

	LomoEMArtBackend *backend = lomo_em_art_backend_new(name, search, cancel, notify, data);
	g_return_val_if_fail(backend != NULL, NULL);

	art_class->priv->backends = g_list_append(art_class->priv->backends, backend);
	g_signal_connect(backend, "finish", (GCallback) backend_finish_cb, art_class);

	return backend;
}

/**
 * lomo_em_art_class_remove_backend:
 * @art_class: An #LomoEMArtClass
 * @backend: (transfer full): An #LomoEMArtBackend
 *
 * Removes backend from all instances of @art_class. @backend should be the
 * object previously created with lomo_em_art_class_add_backend().
 */
void
lomo_em_art_class_remove_backend(LomoEMArtClass *art_class, LomoEMArtBackend *backend)
{
	g_return_if_fail(LOMO_IS_EM_ART_CLASS(art_class));
	g_return_if_fail(LOMO_IS_EM_ART_BACKEND(backend));

	GList *link = g_list_find(art_class->priv->backends, backend);
	g_return_if_fail(link);

	art_class->priv->backends = g_list_remove_link(art_class->priv->backends, link);
	g_object_unref(backend);
	g_list_free(link);
}

/**
 * lomo_em_art_class_try_next_backend:
 * @art_class: An #LomoEMArtClass
 * @search: An #LomoEMArtSearch
 *
 * Tries to find art data for @search in the next registered backend from
 * @art_class.
 * This functions isn't mean for use directly but #LomoEMArt
 */
void
lomo_em_art_class_try_next_backend(LomoEMArtClass *art_class, LomoEMArtSearch *search)
{
	GList *link = lomo_em_art_search_get_bpointer(search);

	// Invalid search
	if (!link || !g_list_find(art_class->priv->backends, link->data))
	{
		g_warning("Search %p is not in our universe, reporting failure", search);
		lomo_em_art_search_set_bpointer(search, NULL);
		lomo_em_art_search_run_callback(search);
		g_object_unref(search);
		return;
	}

	// Jump to the next backend
	if (link->next && link->next->data)
	{
		debug("Moving search %s (%s -> %s)",
			lomo_em_art_search_stringify(search),
			lomo_em_art_backend_get_name(LOMO_EM_ART_BACKEND(link->data)),
			lomo_em_art_backend_get_name(LOMO_EM_ART_BACKEND(link->next->data)));
		LomoEMArtBackend *backend = LOMO_EM_ART_BACKEND(link->next->data);
		lomo_em_art_search_set_bpointer(search, NULL);
		lomo_em_art_search_set_bpointer(search, g_list_next(link));
		lomo_em_art_backend_run(backend, search);
		return;
	}

	// No more backends, report and remove
	debug("Search %s has failed, reporting to caller", lomo_em_art_search_stringify(search));
	LomoEMArt *art = LOMO_EM_ART(lomo_em_art_search_get_domain(search));
	LomoEMArtPrivate *priv = GET_PRIVATE(art);
	priv->searches = g_list_remove(priv->searches, search);

	lomo_em_art_search_set_bpointer(search, NULL);
	lomo_em_art_search_run_callback(search);
	g_object_unref(search);
}

/**
 * lomo_em_art_search:
 * @art: An #LomoEMArt
 * @stream: #LomoStream
 * @callback: Function to be called after search is finished
 * @data: User data for @callback
 *
 * Search art data for @stream. @callback will be called after the search is
 * finished, @callback must NOT free any resource from #LomoEMArt or related
 * objects
 *
 * Returns: (transfer none): #LomoEMArtSearch object representing the search
 */
LomoEMArtSearch*
lomo_em_art_search(LomoEMArt *art, LomoStream *stream, LomoEMArtSearchCallback callback, gpointer data)
{
	g_return_val_if_fail(LOMO_IS_EM_ART(art), NULL);

	LomoEMArtSearch *search = lomo_em_art_search_new((GObject *) art, stream, callback, data);
	g_return_val_if_fail(search != NULL, NULL);

	LomoEMArtPrivate *priv = GET_PRIVATE(art);
	priv->searches = g_list_append(priv->searches, search);

	GList *backends = LOMO_EM_ART_GET_CLASS(art)->priv->backends;

	if (!backends || !backends->data)
	{
		// Report failure
		lomo_em_art_search_run_callback(search);
		priv->searches = g_list_remove(priv->searches, search);
		g_object_unref(search);
		return NULL;
	}

	// We have backends, run them
	lomo_em_art_search_set_bpointer(search, backends);
	lomo_em_art_backend_run(LOMO_EM_ART_BACKEND(backends->data), search);

	return search;
}

/**
 * lomo_em_art_cancel:
 * @art: An #LomoEMArt
 * @search: A running #LomoEMArtSearch
 *
 * Cancels a running search, after cancel/stop any running backend, the
 * callback parameter from lomo_em_art_search() will be called appling the same
 * rules.
 */
void
lomo_em_art_cancel(LomoEMArt *art, LomoEMArtSearch *search)
{
	g_return_if_fail(LOMO_IS_EM_ART(art));
	g_return_if_fail(LOMO_IS_EM_ART(search));

	g_return_if_fail(art == LOMO_EM_ART(lomo_em_art_search_get_domain(search)));

	LomoEMArtClass   *art_class = LOMO_EM_ART_GET_CLASS(art);
	LomoEMArtPrivate *priv      = GET_PRIVATE(art);
	LomoEMArtBackend *backend   = LOMO_EM_ART_BACKEND(((GList *)lomo_em_art_search_get_bpointer(search))->data);

	g_return_if_fail(LOMO_IS_EM_ART_BACKEND(backend));
	g_return_if_fail(g_list_find(art_class->priv->backends, backend) == NULL);

	lomo_em_art_backend_cancel(backend, search);
	priv->searches = g_list_remove(priv->searches, search);
	g_object_unref(search);
}

static void
backend_finish_cb(LomoEMArtBackend *backend, LomoEMArtSearch *search, LomoEMArtClass *art_class)
{
	// Got results
	if (lomo_em_art_search_get_result(search))
	{
		LomoEMArt *art = LOMO_EM_ART(lomo_em_art_search_get_domain(search));
		LomoEMArtPrivate *priv = GET_PRIVATE(art);

		priv->searches = g_list_remove(priv->searches, search);
		lomo_em_art_search_set_bpointer(search, NULL);

		debug("Backend '%s' successfull", lomo_em_art_backend_get_name(backend));
		lomo_em_art_search_run_callback(search);
		g_object_unref(search);
		return;
	}

	// No results
	lomo_em_art_class_try_next_backend(art_class, search);
}
