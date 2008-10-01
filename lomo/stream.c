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
	gboolean all_tags;
};

static gchar *url_decode
(gchar *str);

static gint hex2int
(gchar hex);

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

gchar *
lomo_stream_get_tag_by_id(LomoStream *self, gchar id)
{
	gint i;
	gchar *ret = NULL;

	for (i = 0; tag_fmt_table[i].key != 0; i++)
	{
		if (tag_fmt_table[i].key == id)
		{
			ret = g_strdup_printf(tag_fmt_table[i].fmt, lomo_stream_get_tag(self, tag_fmt_table[i].tag));
			break;
		}
	}
	return ret;
}

/* Functions to decode %NN escaped strings */
gboolean lomo_stream_format
(LomoStream *self, const gchar *fmt, gint max_fails, gint flags, gchar **dest)
{
	GString *string;
	gint i, j, len;
	gboolean key_found = FALSE;
	gchar *ret = NULL;
	gchar *tag;
	
	/* Temporal string for transformations based on flags */
	gchar *tmp;
	
	len = strlen(fmt);
	string = g_string_new_len(NULL, len * 10);
	
	for ( i = 0; i < len; i++ ) {
		/*
		 * Append characters block
		 *
		 * Current characters are simple
		 */
		// g_printf("inspecting char '%c': ", fmt[i]);

		/* Current char is not %, add and go next */
		if ( fmt[i] != '%' ) {
			// g_printf("normal\n");
			string = g_string_append_c(string, fmt[i]);
			continue;
		}

		/* We got an '%' but we are at end of string */
		if ( fmt[i+1] == '\0' ) {
			// g_printf("normal\n");
			string = g_string_append_c(string, fmt[i]);
			continue;
		}
		
		/* 
		 * Substitution block
		 *
		 * Try to match the mark with some metadata
		 * in the stream
		 */

		/* Search in the table for key */
		// g_printf("is key '%c', ", fmt[i+1]);
		for ( j = 0; (tag_fmt_table[j].key) != 0 && !key_found; j++ ) {
			if ( tag_fmt_table[j].key == fmt[i+1] ) {
				tag = (gchar *) g_object_get_data(G_OBJECT(self), tag_fmt_table[j].tag);
				if ( tag  != NULL ) {
					key_found = TRUE;
					g_string_append_printf(string, tag_fmt_table[j].fmt, tag);
				}
			}
		}

		/* If substition was ok, go to next char */
		if ( key_found ) {
			// g_printf("key found (%s)\n", tag);
			i++;
			key_found = FALSE;
			continue;
		} /* else {
			g_printf("key NOT found\n");
		} */
 
		/* Error in substitution, apply fallback methods */
		string = g_string_append_c(string, fmt[i]);
		max_fails--;
		// g_printf("max fails now %d\n", max_fails);
		/* If we didnt reached max errors we can continue processing */
		if ( max_fails >= 0 ) 
			continue;
		
		// g_printf("max fails reached\n");
		/*
		 * Fallback block
		 * 
		 * At this point, we have to discart previous processing
		 * and use uri, stream or filename as base
		 */
		/*
		if ( flags & LOMO_STREAM_LOCATION )
			string = g_string_assign(string, self->stream);
		else if ( flags & LOMO_STREAM_URI )
			string = g_string_assign(string, self->uri);
		*/
		string = g_string_assign(string, g_object_get_data(G_OBJECT(self), LOMO_TAG_URI));
#if 0
		if ( flags & LOMO_STREAM_URI ) {
			string = g_string_assign(string, lomo_stream_get(self, LOMO_TAG_URI));
		}
		else
			/* 
			 * We should handle LOMO_STREAM_FILENAME here
			 * This implies check if file is local and 
			 * call to g_filename_from_uri
			 */
			string = g_string_assign(string, self->stream);
#endif
		/* Stop processing format string */
		break;
	}

	/* Drop the GString */
	ret = string->str;
	g_string_free(string, FALSE);
	
	/* 
	 * Apply flags like dirname or basename 
	 */
	if ( flags & LOMO_STREAM_BASENAME ) {
		tmp = g_path_get_basename(ret);
		g_free(ret);
		ret = tmp;
	}
	if ( flags & LOMO_STREAM_DIRNAME ) {
		tmp = g_path_get_dirname(ret);
		g_free(ret);
		ret = tmp;
	}
	/* Nothing to do with fullname */

	/* 
	 * Apply flags like utf8 and escape
	 */
	if ( flags & LOMO_STREAM_URL_DECODE ) {
		tmp = url_decode(ret);
		g_free(ret);
		ret = tmp;
	}
	
	if ( flags & LOMO_STREAM_URL_DECODE ) {
		tmp = url_decode(ret);
		g_free(ret);
		ret = tmp;
	}
	
	if ( flags & LOMO_STREAM_ESCAPE ) {
		tmp = g_markup_escape_text(ret, -1);
		g_free(ret);
		ret = tmp;
	}

	if ( flags & LOMO_STREAM_UTF8 ) {
		if ( !g_utf8_validate(ret, -1, NULL)) {
			/* Try to convert to utf8 */
			tmp = g_locale_to_utf8(ret, -1, NULL, NULL, NULL);
			if ( tmp ) {
				g_free(ret);
				ret = tmp;
			} else {
				g_warning("Cannot convert string '%s' to UTF8\n", ret);
			}
		}
	}
	*dest = ret;

	if ( max_fails >= 0 )
		return TRUE;
	else
		return FALSE;
}

GType
lomo_tag_get_type(LomoTag tag)
{
	if (g_str_equal(tag, "uri")) 
		return G_TYPE_STRING;
	return gst_tag_get_type(tag);
}


gchar *url_decode(gchar *str) {
	GString *out;
	gint i;
	gint in_len;
	gchar *out_str;
	gboolean hit = FALSE;

	gint a;
	
	in_len = strlen(str);
	out = g_string_new_len(NULL, in_len);

	i = 0;
	while ( i < (in_len - 2) ) {
		if ( str[i] == '%'
			&& g_ascii_isxdigit(str[i+1])
			&& g_ascii_isxdigit(str[i+2])) {
			a = (hex2int(str[i+1]) * 16) + hex2int(str[i+2]);
			out = g_string_append_c(out, a);
			hit = TRUE;
		} else {
			out = g_string_append_c(out, str[i]);
			hit = FALSE;
		}

		if ( hit )
			i += 3;
		else
			i += 1;
	}

	if ( !hit ) {
		out = g_string_append_c(out, str[in_len - 2]);
		out = g_string_append_c(out, str[in_len - 1]);
	}
	
	out_str = out->str;
	g_string_free(out, FALSE);

	return out_str;
}

gint
hex2int(gchar hex)
{
	if ( ('0' <= hex) && (hex <= '9') )
		return hex - '0';

	if ( ('A' <= hex) && (hex <= 'F') )
		return hex - 'A' + 10;

	if ( ('a' <= hex) && (hex <= 'f') )
		return hex - 'a' + 10;

	return 0;
}

