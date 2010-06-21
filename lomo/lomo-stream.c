/*
 * lomo/lomo-stream.c
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

#include <string.h>
#include <gst/gst.h>
#include <lomo/lomo-stream.h>
#include <lomo/lomo-util.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE (LomoStream, lomo_stream, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_STREAM, LomoStreamPrivate))

typedef struct _LomoStreamPrivate LomoStreamPrivate;

struct TagFmt {
	gchar   key;
	LomoTag tag;
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

static void
lomo_stream_dispose (GObject *object)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(LOMO_STREAM(object));

	if (priv->tags)
	{
		g_list_foreach(priv->tags, (GFunc) g_free, NULL);
		g_list_free(priv->tags);
		priv->tags = NULL;
		// g_printf("[-] Disposing stream '%p' ('%s')\n", object, (gchar *) lomo_stream_get_tag((LomoStream *) object, LOMO_TAG_URI));
	}

	if (G_OBJECT_CLASS (lomo_stream_parent_class)->dispose)
		G_OBJECT_CLASS (lomo_stream_parent_class)->dispose (object);
}

static void
lomo_stream_class_init (LomoStreamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoStreamPrivate));

	object_class->dispose = lomo_stream_dispose;
}

static void
lomo_stream_init (LomoStream *self)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(self);
	priv->all_tags = FALSE;
	priv->tags     = NULL;
}

/**
 * lomo_stream_new:
 * @uri: (in) (transfer none): An uri to create a #LomoStream from.
 *
 * Create a new #LomoStream from an uri
 * 
 * Returns: A new #LomoStream
 */
LomoStream*
lomo_stream_new (gchar *uri)
{
	LomoStream *self;
	gint i;

	g_return_val_if_fail(uri != NULL, NULL);
	
	// Check valid URI, more strict methods than this: g_uri_parse_scheme
	for (i = 0; uri[i] != '\0'; i++)
		if ((uri[i] < 20) || (uri[i] > 126))
			return NULL;

	if (strstr(uri, "://") == NULL)
		return NULL;

	// Create instance once URI
	self = g_object_new (LOMO_TYPE_STREAM, NULL);
	g_object_set_data_full(G_OBJECT(self), LOMO_TAG_URI, g_strdup(uri), g_free);

	return self;
}

/**
 * lomo_stream_get_tag:
 * @stream: a #LomoStream
 * @tag: (in) (type gchar*) (transfer none): a #LomoTag
 *
 * Gets a tag from #LomoStream. The returned value is owned by @stream, and
 * should not be modified (Internally it uses g_object_get_data).
 *
 * Return value: (type gchar*) (transfer none): A pointer to the tag
 * value
 */
gchar*
lomo_stream_get_tag(LomoStream *self, LomoTag tag)
{
	g_return_val_if_fail(LOMO_IS_STREAM(self), NULL);
	g_return_val_if_fail(tag, NULL);

	return g_object_get_data((GObject *) self, tag);
}

/**
 * lomo_stream_set_tag:
 * @self: a #LomoStream
 * @tag: (in) (type gchar*) (transfer none): a #LomoTag to set
 * @value: (in) (type gchar*) (transfer none): value for tag, must not be modified. It becomes owned by #LomoStream
 *
 * Sets a tag in a #LomoStream
 */
void
lomo_stream_set_tag(LomoStream *self, LomoTag tag, gpointer value)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(self);
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
 * Gets the list of #LomoTag for a #LomoStream
 *
 * Return value: (element-type utf8) (transfer full): a #GList, it must be freed when no longer needed, data too
 */
GList*
lomo_stream_get_tags(LomoStream *self)
{
	GList *ret = NULL;
	GList *iter = GET_PRIVATE(self)->tags;
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
 */
void
lomo_stream_set_all_tags_flag(LomoStream *self, gboolean value)
{
	GET_PRIVATE(self)->all_tags = value;
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
	return GET_PRIVATE(self)->all_tags;
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
	GET_PRIVATE(self)->failed = value;
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
	return GET_PRIVATE(self)->failed;
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
 * lomo_tag_get_type:
 * @tag: (in) (transfer none): a #LomoTag
 *
 * Queries for the #GType corresponding for the tag
 *
 * Returns: the #GType for tag
 */
GType
lomo_tag_get_g_type(LomoTag tag)
{
	if (g_str_equal(tag, "uri")) 
		return G_TYPE_STRING;
	return gst_tag_get_type(tag);
}

