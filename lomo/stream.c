/*
 * lomo/stream.c
 *
 * Copyright (C) 2004-2009 Eina
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
#include "stream.h"
#include "util.h"

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
}

LomoStream*
lomo_stream_new (gchar *uri)
{
	LomoStream *self;
	gint i;

	// Check valid URI, more strict methods than this: g_uri_parse_scheme
	for (i = 0; uri[i] != '\0'; i++)
	{
		if ((uri[i] < 20) || (uri[i] > 126))
		{
			g_warning("%s is not valid uri", uri);
			return NULL;
		}
	}

	if (strstr(uri, "://") == NULL)
	{
		g_warning("%s is not valid uri", uri);
		return NULL;
	}

	// Create instance once URI
	self = g_object_new (LOMO_TYPE_STREAM, NULL);
	g_object_set_data_full(G_OBJECT(self), LOMO_TAG_URI, g_strdup(uri), g_free);

	return self;
}

void
lomo_stream_set_tag(LomoStream *stream, LomoTag tag, gpointer value)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(stream);
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
	g_object_set_data_full(G_OBJECT(stream), tag, value, g_free);
}

GList*
lomo_stream_get_tags(LomoStream *self)
{
	return g_list_copy(GET_PRIVATE(self)->tags);
}

gboolean
lomo_stream_has_all_tags(LomoStream *self)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(self);
	return priv->all_tags;
}

void lomo_stream_set_all_tags(LomoStream *self, gboolean val)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(self);
	priv->all_tags = val;
}

gboolean
lomo_stream_is_failed(LomoStream *self)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(self);
	return priv->failed;
}

void lomo_stream_set_failed(LomoStream *self, gboolean val)
{
	struct _LomoStreamPrivate *priv = GET_PRIVATE(self);
	priv->failed = val;
}

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

GType
lomo_tag_get_type(LomoTag tag)
{
	if (g_str_equal(tag, "uri")) 
		return G_TYPE_STRING;
	return gst_tag_get_type(tag);
}

