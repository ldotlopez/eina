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
};

static void
lomo_stream_dispose (GObject *object)
{
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
	LomoStream *self = g_object_new (LOMO_TYPE_STREAM, NULL);
	gint i;

	/* Check valid URI */
	i = 0;
	while (uri[i] != '\0') {
		if ((uri[i] < 20) || (uri[i] > 126)) {
			g_warning("%s is not valid uri", uri);
			return NULL;
		}
		i++;
	}

	if (strstr(uri, "://") == NULL) {
		g_warning("%s is not valid uri", uri);
		return NULL;
	}

	/*
 	 * More or less uri _is_ a valid URI now
 	 */
	g_object_set_data_full(G_OBJECT(self), LOMO_TAG_URI, g_strdup(uri), g_free);

	return self;
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

