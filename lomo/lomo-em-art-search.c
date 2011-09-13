/*
 * lomo/lomo-em-art-search.c
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

#include "lomo-em-art-search.h"
#include <gio/gio.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (LomoEMArtSearch, lomo_em_art_search, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART_SEARCH, LomoEMArtSearchPrivate))

#define DEBUG 0
#define DEBUG_PREFIX "LomoEMArtSearch"

#if DEBUG
#	define debug(...) g_debug(DEBUG_PREFIX" "__VA_ARGS__)
#else
#	define debug(...) ;
#endif

typedef struct _LomoEMArtSearchPrivate LomoEMArtSearchPrivate;

enum {
	PROP_0,
	PROP_RESULT
};

struct _LomoEMArtSearchPrivate {
	GObject               *domain;
	LomoStream            *stream;
	LomoEMArtSearchCallback  callback;
	gpointer               callback_data;

	gchar *stringify;

	gpointer bpointer;

	gchar *result;
};

static void lomo_em_art_search_weak_notify_cb(LomoEMArtSearch *search, GObject *old)
{
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));
	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	g_warning(N_("Referenced %s from search '%s' was destroyed."),
		(old == priv->domain) ? "domain" : "stream",
		lomo_em_art_search_stringify(search));
}

static void
lomo_em_art_search_dispose (GObject *object)
{
	LomoEMArtSearch *search = LOMO_EM_ART_SEARCH(object);
	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	if (priv->bpointer)
	{
		g_warning(N_("Search '%s' is running at dispose phase, calling callback and expect problems."), lomo_em_art_search_stringify(search));
		priv->bpointer = NULL;
	}

	if (priv->callback)
	{
		lomo_em_art_search_run_callback(search);
		priv->callback = NULL;
	}

	if (priv->domain)
	{
		g_object_weak_unref((GObject *) priv->domain, (GWeakNotify) lomo_em_art_search_weak_notify_cb, search);
		g_object_unref(priv->domain);
		priv->domain = NULL;
	}

	if (priv->stream)
	{
		g_object_weak_unref((GObject *) priv->stream, (GWeakNotify) lomo_em_art_search_weak_notify_cb, search);
		g_object_unref(priv->stream);
		priv->stream = NULL;
	}

	if (priv->stringify)
	{
		g_free(priv->stringify);
		priv->stringify = NULL;
	}

	if (priv->result)
	{
		g_free(priv->result);
		priv->result = NULL;
	}

	G_OBJECT_CLASS (lomo_em_art_search_parent_class)->dispose (object);
}

static void
eina_art_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_RESULT:
		g_value_set_string(value, lomo_em_art_search_get_result(LOMO_EM_ART_SEARCH(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		return;
	}
}

static void
lomo_em_art_search_class_init (LomoEMArtSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoEMArtSearchPrivate));

	object_class->dispose = lomo_em_art_search_dispose;
	object_class->get_property = eina_art_get_property;

	/**
	 * LomoEMArtSearch:result:
	 *
	 * Result of the search. If search already has been finished and this
	 * property is %NULL this means that search has been failed.
	 */
	g_object_class_install_property(object_class, PROP_RESULT,
		g_param_spec_string("result", "result", "result",
		NULL, G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

static void
lomo_em_art_search_init (LomoEMArtSearch *self)
{
}

/**
 * lomo_em_art_search_new:
 * @domain: Domain of the search
 * @stream: #LomoStream relative to the search
 * @callback: Function to be called after search is finished
 * @callback_data: Data to pass to @callback
 *
 * Creates (but not initiates) a new art search. This function is not mean to
 * be used directly but #EinaArt.
 */
LomoEMArtSearch*
lomo_em_art_search_new (GObject *domain, LomoStream *stream, LomoEMArtSearchCallback callback, gpointer callback_data)
{
	g_return_val_if_fail(G_IS_OBJECT(domain), NULL);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), NULL);
	g_return_val_if_fail(callback, NULL);

	LomoEMArtSearch *search = g_object_new (EINA_TYPE_ART_SEARCH, NULL);
	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	priv->domain = g_object_ref(domain);
	priv->stream = stream;
	g_object_ref(priv->stream);
	priv->callback = callback;
	priv->callback_data = callback_data;

	priv->result = NULL;

	g_object_weak_ref((GObject *) priv->stream, (GWeakNotify) lomo_em_art_search_weak_notify_cb, search);
	g_object_weak_ref((GObject *) priv->domain, (GWeakNotify) lomo_em_art_search_weak_notify_cb, search);

	return search;
}

/**
 * lomo_em_art_search_get_domain:
 * @search: An #LomoEMArtSearch
 *
 * Gets the domain of the search. Meant for be used by #EinaArt
 *
 * Returns: (transfer none): The domain
 */
GObject *
lomo_em_art_search_get_domain(LomoEMArtSearch *search)
{
	return GET_PRIVATE(search)->domain;
}

/**
 * lomo_em_art_search_get_stream:
 * @search: An #LomoEMArtSearch
 *
 * Gets the stream of the search. Meant for be used by #EinaArt
 *
 * Returns: (transfer none): The #LomoStream
 */
LomoStream *
lomo_em_art_search_get_stream(LomoEMArtSearch *search)
{
	return GET_PRIVATE(search)->stream;
}

/**
 * lomo_em_art_search_set_bpointer:
 * @search: An #LomoEMArtSearch
 * @bpointer: (transfer none): The pointer
 *
 * Internal function, dont use.
 */
void
lomo_em_art_search_set_bpointer(LomoEMArtSearch *search, gpointer bpointer)
{
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));
	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	if (bpointer && priv->bpointer)
	{
		g_warning(N_("Trying to set a bpointer while search '%s' already has a bpointer, this is a bug. You should set bpointer to NULL before trying to do this."),
			lomo_em_art_search_stringify(search));
		g_return_if_fail(priv->bpointer == NULL);
	}

	priv->bpointer = bpointer;
}

/**
 * lomo_em_art_search_get_bpointer:
 * @search: An #LomoEMArtSearch
 *
 * Internal function, dont use.
 *
 * Returns: (transfer none): The pointer
 */
gpointer
lomo_em_art_search_get_bpointer(LomoEMArtSearch *search)
{
	return GET_PRIVATE(search)->bpointer;
}

/**
 * lomo_em_art_search_set_result:
 * @search: The #EinaSearch
 * @result: URI for art data
 *
 * Stores result into @search.
 * This functions is meant to by used by #EinaArtBackend implementations to
 * store result into #LomoEMArtSearch
 */
void
lomo_em_art_search_set_result(LomoEMArtSearch *search, const gchar *result)
{
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));
	g_return_if_fail(result != NULL);

	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	if (result && priv->result)
	{
		g_warning(N_("Trying to set a result in search '%s' while it already has one, this is a bug and will be ignored"),
			lomo_em_art_search_stringify(search));
		g_return_if_fail(priv->result == NULL);
	}

	priv->result = g_strdup(result);
	g_object_notify(G_OBJECT(search), "result");
}

/**
 * lomo_em_art_search_get_result:
 * @search: An #LomoEMArtSearch
 *
 * Gets the result stored into @search. This function is usefull inside the
 * callback function from lomo_em_art_search() to get the result of the search
 *
 * Returns: The art URI for @search
 */
const gchar *
lomo_em_art_search_get_result(LomoEMArtSearch *search)
{
	g_return_val_if_fail(LOMO_IS_EM_ART_SEARCH(search), NULL);

	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);
	return priv->result;
}

/**
 * lomo_em_art_search_stringify:
 * @search: An #LomoEMArtSearch
 *
 * Transforms @search into a string just for debugging purposes
 *
 * Returns: The strigified form of @search
 */
const gchar*
lomo_em_art_search_stringify(LomoEMArtSearch *search)
{
	g_return_val_if_fail(LOMO_IS_EM_ART_SEARCH(search), NULL);
	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	if (!priv->stringify)
	{
		gchar *unescape = g_uri_unescape_string(lomo_stream_get_tag(priv->stream, LOMO_TAG_URI), NULL);
		priv->stringify = g_path_get_basename(unescape);
		g_free(unescape);
		g_return_val_if_fail(priv->stringify, NULL);
	}
	return priv->stringify;
}

/**
 * lomo_em_art_search_run_callback:
 * @search: An #LomoEMArtSearch
 *
 * Runs the callback associated with @search 
 */
void
lomo_em_art_search_run_callback(LomoEMArtSearch *search)
{
	g_return_if_fail(LOMO_IS_EM_ART_SEARCH(search));

	LomoEMArtSearchPrivate *priv = GET_PRIVATE(search);

	g_return_if_fail(priv->bpointer == NULL);

	if (!priv->callback)
	{
		g_warning("%s has been called multiple times on %s, ignoring.",
			__FUNCTION__,
			lomo_em_art_search_stringify(search));
		return;
	}

	debug("Run callback for search %s", lomo_em_art_search_stringify(search));
	priv->callback(search, priv->callback_data);
	priv->callback = NULL;
}

