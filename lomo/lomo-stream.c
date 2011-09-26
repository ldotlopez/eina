/*
 * lomo/lomo-stream.c
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

#include "lomo-stream.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gst/gst.h>
#include "lomo.h"

G_DEFINE_TYPE (LomoStream, lomo_stream, G_TYPE_OBJECT)

struct TagFmt {
	gchar   key;
	const gchar *tag;
	gchar  *fmt;
};

static struct TagFmt tag_fmt_table [] = {
	{ 'a', LOMO_TAG_ARTIST       , "%s"  },
	{ 'b', LOMO_TAG_ALBUM        , "%s"  },
	{ 't', LOMO_TAG_TITLE        , "%s"  },
	{ 'g', LOMO_TAG_GENRE        , "%s"  },
	{ 'n', LOMO_TAG_TRACK_NUMBER , "%02d"},
	{ 'd', LOMO_TAG_DATE         , "%s"  },
	{   0, LOMO_TAG_INVALID      , "%p"  }
};

struct _LomoStreamPrivate {
	gboolean all_tags, failed;
	GList *tags;
};

enum {
	PROP_0,
	PROP_URI
};

enum {
	SIGNAL_EXTENDED_METADATA_UPDATED,
	LAST_SIGNAL
};
guint lomo_stream_signals[LAST_SIGNAL] = { 0 };

static void
stream_set_uri(LomoStream *self, const gchar *uri);

static void
lomo_stream_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	LomoStream *self = LOMO_STREAM(object);
	switch (property_id)
	{
	case PROP_URI:
		g_value_set_static_string(value, lomo_stream_get_tag(self, LOMO_TAG_URI));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static void
lomo_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	LomoStream *self = LOMO_STREAM(object);
	switch (property_id)
	{
	case PROP_URI:
		stream_set_uri(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static void
lomo_stream_dispose (GObject *object)
{
	struct _LomoStreamPrivate *priv = LOMO_STREAM(object)->priv;

	if (priv->tags)
	{
		g_list_foreach(priv->tags, (GFunc) g_free, NULL);
		g_list_free(priv->tags);
		priv->tags = NULL;
	}

	if (G_OBJECT_CLASS (lomo_stream_parent_class)->dispose)
		G_OBJECT_CLASS (lomo_stream_parent_class)->dispose (object);
}

static void
lomo_stream_class_init (LomoStreamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoStreamPrivate));

	object_class->get_property = lomo_stream_get_property;
	object_class->set_property = lomo_stream_set_property;
	object_class->dispose = lomo_stream_dispose;

	/**
	 * LomoStream:uri:
	 *
	 * URI corresponding to the stream
	 */
	g_object_class_install_property(object_class, PROP_URI,
		g_param_spec_string("uri", "uri", "uri",
			NULL,  G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));

	/**
	 * LomoStream::extended-metadata-updated:
	 * @stream: the object that recieved the signal
	 * @key: Name of the updated metadata key
	 *
	 * Emitted when extended metadata (cover, lyrics, etc...) is updated
	 * for the stream
	 */
	lomo_stream_signals[SIGNAL_EXTENDED_METADATA_UPDATED] =
		g_signal_new ("extended-metadata-updated",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoStreamClass, extended_metadata_updated),
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING);
}

static void
lomo_stream_init (LomoStream *self)
{
	LomoStreamPrivate *priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), LOMO_TYPE_STREAM, LomoStreamPrivate);

	priv->all_tags = FALSE;
	priv->tags     = NULL;
}

/**
 * lomo_stream_new:
 * @uri: An uri to create a #LomoStream from.
 *
 * Create a new #LomoStream from an uri
 * 
 * Returns: A new #LomoStream
 */
LomoStream*
lomo_stream_new (const gchar *uri)
{
	return g_object_new (LOMO_TYPE_STREAM, "uri", uri, NULL);
}

static void
stream_set_uri(LomoStream *self, const gchar *uri)
{
	g_return_if_fail(LOMO_IS_STREAM(self));
	g_return_if_fail(uri != NULL);

	// Check valid URI, more strict methods than this: g_uri_parse_scheme
	for (guint i = 0; uri[i] != '\0'; i++)
		if ((uri[i] < 20) || (uri[i] > 126))
			g_warning(_("'%s' is not a valid URI"), uri);

	if (!uri && (strstr(uri, "://") == NULL))
		g_warning(_("'%s' is not a valid URI"), uri);

	g_object_set_data_full(G_OBJECT(self), LOMO_TAG_URI, g_strdup(uri), g_free);
}

gchar *
lomo_stream_string_parser_cb(gchar tag_key, LomoStream *self)
{
	gchar *ret = NULL;
	gchar *tag_str = lomo_stream_get_tag_by_id(self, tag_key);

	if (tag_str != NULL)
	{
		ret = g_markup_escape_text(tag_str, -1);
		g_free(tag_str);
	}

	if ((tag_key == 't') && (ret == NULL))
	{
		const gchar *tmp = lomo_stream_get_tag(self, LOMO_TAG_URI);
		gchar *tmp2 = g_uri_unescape_string(tmp, NULL);
		ret = g_path_get_basename(tmp2);
		g_free(tmp2);
	}
	return ret;
}

/**
 * lomo_stream_get_tag:
 * @self: a #LomoStream
 * @tag: a #LomoTag
 *
 * Gets a tag from #LomoStream. The returned value is owned by @stream, and
 * should not be modified (Internally it uses g_object_get_data).
 *
 * Returns: A pointer to the tag value
 */
const gchar*
lomo_stream_get_tag(LomoStream *self, const gchar *tag)
{
	g_return_val_if_fail(LOMO_IS_STREAM(self), NULL);
	g_return_val_if_fail(tag, NULL);

	const gchar *ret = g_object_get_data((GObject *) self, tag);
	return ret;
}

/**
 * lomo_stream_set_tag:
 * @self: a #LomoStream
 * @tag: (in) (type gchar*) (transfer none): a #const gchar *to set
 * @value: (in) (type gchar*) (transfer none): value for tag, must not be modified. It becomes owned by #LomoStream
 *
 * Sets a tag in a #LomoStream
 */
void
lomo_stream_set_tag(LomoStream *self, const gchar *tag, gpointer value)
{
	LomoStreamPrivate *priv = self->priv;
	GList *link = g_list_find_custom(priv->tags, tag, (GCompareFunc) strcmp);

	if (tag != NULL)
	{
		if (link != NULL)
		{
			g_free(link->data);
			link->data = g_strdup(tag);
		}
		else
			priv->tags = g_list_prepend(priv->tags, g_strdup(tag));
	}
	else
	{
		priv->tags = g_list_remove_link(priv->tags, link);
		g_free(link->data);
		g_list_free(link);
	}
	g_object_set_data_full(G_OBJECT(self), tag, value, g_free);
}

/**
 * lomo_stream_get_tags:
 * @self: a #LomoStream
 *
 * Gets the list of #const gchar *for a #LomoStream
 *
 * Return value: (element-type utf8) (transfer full): a #GList, it must be freed when no longer needed, data too
 */
GList*
lomo_stream_get_tags(LomoStream *self)
{
	GList *ret = NULL;
	GList *iter = self->priv->tags;
	while (iter)
	{
		ret = g_list_prepend(ret, g_strdup((gchar *) iter->data));
		iter = iter->next;
	}
	return ret;
}

/**
 * lomo_stream_set_all_tags_flag:
 * @self: a #LomoStream
 * @value: (in): value for flag
 *
 * Sets the all_tags flag to value
 **/
void
lomo_stream_set_all_tags_flag(LomoStream *self, gboolean value)
{
	self->priv->all_tags = value;
}

/**
 * lomo_stream_get_all_tags_flag:
 * @self: a #LomoStream
 *
 * Gets value of all_tags flag
 *
 * Returns: the value of all_tags flag
 */
gboolean
lomo_stream_get_all_tags_flag(LomoStream *self)
{
	return self->priv->all_tags;
}

/**
 * lomo_stream_set_failed_flag:
 * @self: a #LomoStream
 * @value: (in): value for flag
 *
 * Sets the failed flag to value
 */
void
lomo_stream_set_failed_flag(LomoStream *self, gboolean value)
{
	self->priv->failed = value;
}

/**
 * lomo_stream_get_failed_flag:
 * @self: a #LomoStream
 *
 * Gets value of failed flag
 *
 * Returns: the value of failed flag
 */
gboolean
lomo_stream_get_failed_flag(LomoStream *self)
{
	return self->priv->failed;
}

static void
destroy_gvalue(GValue *value)
{
	if (value != NULL)
	{
		g_return_if_fail(G_IS_VALUE(value));
		g_value_unset(value);
		g_free(value);
	}
}

/**
 * lomo_stream_set_extended_metadata:
 * @self: A #LomoStream
 * @key: Key
 * @value: (transfer full): Value to store
 *
 * Adds (or replaces) the value for the extended metadata for key
 */
void
lomo_stream_set_extended_metadata(LomoStream *self, const gchar *key, GValue *value)
{
	g_return_if_fail(LOMO_IS_STREAM(self));
	g_return_if_fail(key != NULL);
	g_return_if_fail(G_IS_VALUE(value));

	gchar *k = g_strconcat("x-lomo-extended-metadata-", key, NULL);
	g_object_set_data_full(G_OBJECT(self), k, value, (GDestroyNotify) destroy_gvalue);
	g_free(k);

	g_signal_emit(self, lomo_stream_signals[SIGNAL_EXTENDED_METADATA_UPDATED], 0, key);
}

/**
 * lomo_stream_get_extended_metadata:
 * @self: A #LomoStream
 * @key: A Key
 *
 * See g_object_get_data()
 *
 * Returns: (transfer none): The value associated with the key
 */
GValue*
lomo_stream_get_extended_metadata(LomoStream *self, const gchar *key)
{
	g_return_val_if_fail(LOMO_IS_STREAM(self), NULL);

	gchar *k = g_strconcat("x-lomo-extended-metadata-", key, NULL);
	GValue *ret = g_object_get_data(G_OBJECT(self), k);
	g_free(k);

	return ret;
}

/**
 * lomo_stream_get_extended_metadata_as_string
 * @self: A #LomoStream
 * @key: A Key
 *
 * See lomo_stream_get_extended_metadata()
 *
 * Returns: (transfer none): The value associated with the key
 */
const gchar *
lomo_stream_get_extended_metadata_as_string(LomoStream *self, const gchar *key)
{
	GValue *v = lomo_stream_get_extended_metadata(self, key);
	return (v ? g_value_get_string(v) : NULL);
}

/**
 * lomo_stream_get_tag_by_id:
 * @self: a #LomoStream
 * @id: (in): identifier for tag (t = title, b = album, etc...)
 *
 * Gets the tag value as string for the matching id
 *
 * Retuns: the tag value as string
 */
gchar *
lomo_stream_get_tag_by_id(LomoStream *self, gchar id)
{
	gint i;
	gchar *ret = NULL;

	for (i = 0; tag_fmt_table[i].key != 0; i++)
	{
		if (tag_fmt_table[i].key == id)
		{
			const gchar *tag_str = lomo_stream_get_tag(self, tag_fmt_table[i].tag);
			if (tag_str == NULL)
				return NULL;
			ret = g_strdup_printf(tag_fmt_table[i].fmt, tag_str);
			break;
		}
	}
	return ret;
}

/*
 * lomo_tag_get_gtype:
 * @tag: (in) (transfer none): a #LomoTag
 *
 * Queries for the #GType corresponding for the tag
 *
 * Returns: the #GType for tag
 */
GType
lomo_tag_get_gtype(const gchar *tag)
{
	if (g_str_equal(tag, "uri")) 
		return G_TYPE_STRING;
	return gst_tag_get_type(tag);
}

