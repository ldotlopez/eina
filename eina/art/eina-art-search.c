/*
 * eina/art/eina-art-search.c
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

#include "eina-art-search.h"
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gel/gel.h>

G_DEFINE_TYPE (EinaArtSearch, eina_art_search, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ART_SEARCH, EinaArtSearchPrivate))

#define DEBUG_PREFIX "EinaArtSearch"
#define debug(...) g_debug(DEBUG_PREFIX" "__VA_ARGS__)

typedef struct _EinaArtSearchPrivate EinaArtSearchPrivate;

enum {
	PROP_0,
	PROP_RESULT
};

struct _EinaArtSearchPrivate {
	GObject               *domain;
	LomoStream            *stream;
	EinaArtSearchCallback  callback;
	gpointer               callback_data;

	gchar *stringify;

	gpointer bpointer;

	gchar *result;
	GdkPixbuf *result_pb;
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

	if (priv->result)
	{
		g_free(priv->result);
		priv->result = NULL;
	}

	if (priv->result_pb)
	{
		g_object_unref(priv->result_pb);
		priv->result = NULL;
	}

	G_OBJECT_CLASS (eina_art_search_parent_class)->dispose (object);
}

static void
eina_art_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_RESULT:
		g_value_set_string(value, eina_art_search_get_result(EINA_ART_SEARCH(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		return;
	}
}

static void
eina_art_search_class_init (EinaArtSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaArtSearchPrivate));

	object_class->dispose = eina_art_search_dispose;
	object_class->get_property = eina_art_get_property;

	/**
	 * EinaArtSearch:result:
	 *
	 * Result of the search. If search already has been finished and this
	 * property is %NULL this means that search has been failed.
	 */
	g_object_class_install_property(object_class, PROP_RESULT,
		g_param_spec_string("result", "result", "result",
		NULL, G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

static void
eina_art_search_init (EinaArtSearch *self)
{
}

/**
 * eina_art_search_new:
 * @domain: Domain of the search
 * @stream: #LomoStream relative to the search
 * @callback: Function to be called after search is finished
 * @callback_data: Data to pass to @callback
 *
 * Creates (but not initiates) a new art search. This function is not mean to
 * be used directly but #EinaArt.
 */
EinaArtSearch*
eina_art_search_new (GObject *domain, LomoStream *stream, EinaArtSearchCallback callback, gpointer callback_data)
{
	g_return_val_if_fail(G_IS_OBJECT(domain), NULL);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), NULL);
	g_return_val_if_fail(callback, NULL);

	EinaArtSearch *search = g_object_new (EINA_TYPE_ART_SEARCH, NULL);
	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	priv->domain = g_object_ref(domain);
	priv->stream = stream;
	g_object_ref(priv->stream);
	priv->callback = callback;
	priv->callback_data = callback_data;

	priv->result = NULL;

	g_object_weak_ref((GObject *) priv->stream, (GWeakNotify) eina_art_search_weak_notify_cb, search);
	g_object_weak_ref((GObject *) priv->domain, (GWeakNotify) eina_art_search_weak_notify_cb, search);

	return search;
}

/**
 * eina_art_search_get_domain:
 * @search: An #EinaArtSearch
 *
 * Gets the domain of the search. Meant for be used by #EinaArt
 *
 * Returns: (transfer none): The domain
 */
GObject *
eina_art_search_get_domain(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->domain;
}

/**
 * eina_art_search_get_stream:
 * @search: An #EinaArtSearch
 *
 * Gets the stream of the search. Meant for be used by #EinaArt
 *
 * Returns: (transfer none): The #LomoStream
 */
LomoStream *
eina_art_search_get_stream(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->stream;
}

/**
 * eina_art_search_set_bpointer:
 * @search: An #EinaArtSearch
 * @bpointer: (transfer none): The pointer
 *
 * Internal function, dont use.
 */
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

/**
 * eina_art_search_get_bpointer:
 * @search: An #EinaArtSearch
 *
 * Internal function, dont use.
 *
 * Returns: (transfer none): The pointer
 */
gpointer
eina_art_search_get_bpointer(EinaArtSearch *search)
{
	return GET_PRIVATE(search)->bpointer;
}

/**
 * eina_art_search_set_result:
 * @search: The #EinaSearch
 * @result: URI for art data
 *
 * Stores result into @search.
 * This functions is meant to by used by #EinaArtBackend implementations to
 * store result into #EinaArtSearch
 */
void
eina_art_search_set_result(EinaArtSearch *search, const gchar *result)
{
	g_return_if_fail(EINA_IS_ART_SEARCH(search));
	g_return_if_fail(result != NULL);

	EinaArtSearchPrivate *priv = GET_PRIVATE(search);

	if (result && priv->result)
	{
		g_warning(N_("Trying to set a result in search '%s' while it already has one, this is a bug and will be ignored"),
			eina_art_search_stringify(search));
		g_return_if_fail(priv->result == NULL);
	}

	priv->result = g_strdup(result);
	g_object_notify(G_OBJECT(search), "result");
}

/**
 * eina_art_search_get_result:
 * @search: An #EinaArtSearch
 *
 * Gets the result stored into @search. This function is usefull inside the
 * callback function from eina_art_search() to get the result of the search
 *
 * Returns: The art URI for @search
 */
const gchar *
eina_art_search_get_result(EinaArtSearch *search)
{
	g_return_val_if_fail(EINA_IS_ART_SEARCH(search), NULL);

	EinaArtSearchPrivate *priv = GET_PRIVATE(search);
	return priv->result;
}

/**
 * eina_art_search_get_result_as_pixbuf:
 * @search: An #EinaArtSearch
 *
 * Gets the result of the search as a #GdkPixbuf
 *
 * See also: eina_art_search_get_result()
 * Returns: (transfer full): The #GdkPixbuf
 */
GdkPixbuf *
eina_art_search_get_result_as_pixbuf(EinaArtSearch *search)
{
	g_return_val_if_fail(EINA_IS_ART_SEARCH(search), NULL);

	EinaArtSearchPrivate *priv = GET_PRIVATE(search);
	if (!priv->result)
		return NULL;

	GError *error = NULL;

	GFile *f = g_file_new_for_uri(priv->result);
	GInputStream *i_stream = G_INPUT_STREAM(g_file_read(f, NULL, &error));
	if (!i_stream)
	{
		g_warning(_("Unable to open URI '%s': '%s'"), priv->result, error->message);
		g_error_free(error);
		g_object_unref(f);
		return NULL;
	}
	g_object_unref(f);

	GdkPixbuf *ret = gdk_pixbuf_new_from_stream(i_stream, NULL, &error);
	if (!ret)
	{
		g_warning(_("Unable to read URI '%s': '%s'"), priv->result, error->message);
		g_error_free(error);
		g_object_unref(i_stream);
		return NULL;
	}
	g_object_unref(i_stream);

	return ret;
}

/**
 * eina_art_search_stringify:
 * @search: An #EinaArtSearch
 *
 * Transforms @search into a string just for debugging purposes
 *
 * Returns: The strigified form of @search
 */
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

/**
 * eina_art_search_run_callback:
 * @search: An #EinaArtSearch
 *
 * Runs the callback associated with @search 
 */
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

