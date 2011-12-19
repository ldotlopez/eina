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
};

static struct TagFmt tag_fmt_table [] = {
	{ 'a', LOMO_TAG_ARTIST       },
	{ 'b', LOMO_TAG_ALBUM        },
	{ 't', LOMO_TAG_TITLE        },
	{ 'g', LOMO_TAG_GENRE        },
	{ 'n', LOMO_TAG_TRACK_NUMBER },
	{ 'd', LOMO_TAG_DATE         },
	{   0, LOMO_TAG_INVALID      }
};

struct _LomoStreamPrivate {
	gboolean all_tags, failed;
	GList *tags;
};

enum {
	PROP_0,
	PROP_URI,
	PROP_LENGTH
};

enum {
	SIGNAL_EXTENDED_METADATA_UPDATED,
	LAST_SIGNAL
};
guint lomo_stream_signals[LAST_SIGNAL] = { 0 };

static void
stream_set_uri(LomoStream *self, const gchar *uri);

static void
destroy_gvalue(GValue *value);

static void
lomo_stream_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	LomoStream *self = LOMO_STREAM(object);
	switch (property_id)
	{
	case PROP_URI:
		g_value_set_static_string(value, lomo_stream_get_uri(self));
		break;
	case PROP_LENGTH:
		g_value_set_int64(value, lomo_stream_get_length(self));
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

	g_object_class_install_property(object_class, PROP_LENGTH,
		g_param_spec_int64("length", "length", "length",
			-1, G_MAXINT64, -1,  G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

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

	GValue v = { 0 };
	g_value_set_static_string(g_value_init(&v, G_TYPE_STRING), uri);
	lomo_stream_set_tag(self, "uri", &v);
	g_value_reset(&v);

	g_object_notify(G_OBJECT(self), "uri");
}

/**
 * lomo_stream_string_parser_cb:
 * @tag_key: Key for tag
 * @self: A #LomoStream
 *
 * Default callback for format a stream, see gel_str_parse()
 *
 * Returns: (transfer full): String for @tag_key
 */
gchar *
lomo_stream_string_parser_cb(gchar tag_key, LomoStream *self)
{
	gchar *ret = NULL;
	gchar *tag_str = lomo_stream_get_tag_by_id(self, tag_key);

	if (tag_str != NULL)
		return tag_str;

	if ((tag_key == 't') && (ret == NULL))
	{
		const gchar *tmp = lomo_stream_get_uri(self);
		gchar *tmp2 = g_uri_unescape_string(tmp, NULL);
		ret = g_path_get_basename(tmp2);
		g_free(tmp2);
	}
	return ret;
}

/**
 * lomo_stream_strdup_tag_value:
 * @self: A #LomoStream
 * @tag: A #LomoTag
 *
 * Get the @tag as string
 *
 * Returns: (transfer full): Tag as string. Must be free
 */
gchar*
lomo_stream_strdup_tag_value(LomoStream *self, const gchar *tag)
{
	g_return_val_if_fail(LOMO_IS_STREAM(self), NULL);
	g_return_val_if_fail(tag, NULL);
	
	const GValue *v = lomo_stream_get_tag(self, tag);
	if (!v)
		return NULL;

	GValue v2 = { 0 };
	g_value_init(&v2, G_TYPE_STRING);
	if (g_value_transform(v, &v2))
	{
		gchar *ret = g_strdup(g_value_get_string(&v2));
		g_value_unset(&v2);
		return ret;
	}
	else
		return g_strdup_value_contents(v);
}

/**
 * lomo_stream_get_tag:
 * @self: A #LomoStream
 * @tag: A #LomoTag
 *
 * Gets the value for @tag
 *
 * Returns: (transfer none): #GValue for @tag
 */
const GValue*
lomo_stream_get_tag(LomoStream *self, const gchar *tag)
{
	g_return_val_if_fail(LOMO_IS_STREAM(self), NULL);
	g_return_val_if_fail(tag, NULL);

	LomoStreamPrivate *priv = self->priv;
	if (!g_list_find_custom(priv->tags, tag, (GCompareFunc) strcmp))
		return NULL;

	const GValue *ret = g_object_get_data((GObject *) self, tag);

	if (ret)
		g_return_val_if_fail(G_IS_VALUE(ret), NULL);

	return ret;
}

/**
 * lomo_stream_set_tag:
 * @self: A #LomoStream
 * @tag: A #LomoTag
 * @value: (transfer none): A #GValue for the value
 *
 * Sets the value for @tag
 */
void
lomo_stream_set_tag(LomoStream *self, const gchar *tag, const GValue *value)
{
	g_return_if_fail(LOMO_IS_STREAM(self));
	g_return_if_fail(tag);
	if (value)
		g_return_if_fail(G_IS_VALUE(value));

	LomoStreamPrivate *priv = self->priv;

	GValue *v2 = g_value_init(g_new0(GValue, 1), G_VALUE_TYPE(value));
	g_value_copy(value, v2);

	g_object_set_data_full((GObject *) self, tag, v2, (GDestroyNotify) destroy_gvalue);

	GList *link = g_list_find_custom(priv->tags, tag, (GCompareFunc) strcmp);

	// Add tag to tag list 
	if ((value != NULL) && (link == NULL))
		priv->tags = g_list_prepend(priv->tags, g_strdup(tag));

	// Remove tag from list
	if ((value == NULL) && (link != NULL))
	{
		priv->tags = g_list_remove_link(priv->tags, link);
		g_free(link->data);
		g_list_free(link);
	}
}

/**
 * lomo_stream_get_tags:
 * @self: a #LomoStream
 *
 * Gets the list of tags
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

/**
 * lomo_stream_set_extended_metadata:
 * @self: A #LomoStream
 * @key: (transfer none): Key
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
 * lomo_stream_set_extended_metadata:
 * @self: A #LomoStream
 * @key: (transfer none): Key
 * @value: (transfer none): Value to store
 *
 * Adds (or replaces) the value for the extended metadata for key
 */
void
lomo_stream_set_extended_metadata_as_string(LomoStream *self, const gchar *key, const gchar *value)
{
	g_return_if_fail(LOMO_IS_STREAM(self));
	g_return_if_fail(key);
	g_return_if_fail(value);

	GValue *v = g_new0(GValue, 1);
	g_value_set_string(g_value_init(v, G_TYPE_STRING), value);
	lomo_stream_set_extended_metadata(self, key, v);
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
GValue *
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
 * Returns: (transfer none): The value associated with the key as a string or
 *          %NULL if not found or key is not a string
 */
const gchar *
lomo_stream_get_extended_metadata_as_string(LomoStream *self, const gchar *key)
{
	GValue *v = lomo_stream_get_extended_metadata(self, key);
	return (v && G_VALUE_HOLDS_STRING(v) ? g_value_get_string(v) : NULL);
}

/**
 * lomo_stream_get_tag_by_id:
 * @self: a #LomoStream
 * @id: identifier for tag (t = title, b = album, etc...)
 *
 * Gets the tag value as string for the matching id
 *
 * Returns: (transfer full): the tag value as string, must be free
 */
gchar *
lomo_stream_get_tag_by_id(LomoStream *self, gchar id)
{
	for (guint i = 0; tag_fmt_table[i].key != 0; i++)
		if (tag_fmt_table[i].key == id)
			return lomo_stream_strdup_tag_value(self, tag_fmt_table[i].tag);

	return NULL;
}

/**
 * lomo_stream_get_length:
 * self: A #LomoStream
 *
 * Gets the lenght or duration of @self in nanoseconds
 *
 * Returns: The length of stream or -1 if undefined
 */
gint64
lomo_stream_get_length(LomoStream *self)
{
	g_return_val_if_fail(LOMO_IS_STREAM(self), -1);
	gpointer p = g_object_get_data(G_OBJECT(self), "length");
	if (p)
		return *((gint64*) p);
	else
		return -1;
}

/**
 * lomo_stream_set_length:
 * @self: A #LomoStream
 * @length: private
 *
 * This function is private. Don't use it unless you know what are you doing
 */
void
lomo_stream_set_length(LomoStream *self, gint64 length)
{
	g_return_if_fail(LOMO_IS_STREAM(self));
	g_return_if_fail(length >= 0);

	gint64 *v = g_new0(gint64, 1);
	*v = length;
	g_object_set_data_full(G_OBJECT(self), "length", v, g_free);

	g_object_notify(G_OBJECT(self), "length");
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
