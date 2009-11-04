/*
 * lomo/lomo-stream.h
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

#ifndef _LOMO_STREAM_H
#define _LOMO_STREAM_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef const gchar* LomoTag;

#define LOMO_TYPE_STREAM lomo_stream_get_type()

#define LOMO_STREAM(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_STREAM, LomoStream))

#define LOMO_STREAM_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), LOMO_TYPE_STREAM, LomoStreamClass))

#define LOMO_IS_STREAM(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_STREAM))

#define LOMO_IS_STREAM_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOMO_TYPE_STREAM))

#define LOMO_STREAM_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), LOMO_TYPE_STREAM, LomoStreamClass))

typedef struct {
	GObject parent;
} LomoStream;

typedef struct {
	GObjectClass parent_class;
} LomoStreamClass;

GType lomo_stream_get_type (void);

LomoStream* lomo_stream_new (gchar *uri);

#ifdef LIBLOMO_COMPILATION
void lomo_stream_set_all_tags_flag(LomoStream *self, gboolean val);
void lomo_stream_set_failed_flag  (LomoStream *self, gboolean val);
#endif

gboolean lomo_stream_get_all_tags_flag(LomoStream *self);
gboolean lomo_stream_get_failed_flag  (LomoStream *self);

#define lomo_stream_get_tag(stream,tag)  g_object_get_data(G_OBJECT(stream), tag)
gchar*  lomo_stream_get_tag_by_id(LomoStream *self, gchar id);
void    lomo_stream_set_tag      (LomoStream *self, LomoTag tag, gpointer value);
GList*  lomo_stream_get_tags     (LomoStream *self);

GType   lomo_tag_get_g_type(LomoTag tag);

/* 
 * To (re-)generate this list, run:
 *
 * echo -e "#define LOMO_TAG_INVALID NULL\n#define LOMO_TAG_URI \"uri\"" && \
 * grep '#define GST_TAG' /usr/include/gstreamer-0.10/gst/gsttaglist.h | \
 * grep -v '(' | \
 * sed -e 's/GST_/LOMO_/g' 
 */
#define LOMO_TAG_INVALID NULL
#define LOMO_TAG_URI "uri"
#define LOMO_TAG_TITLE                  "title"
#define LOMO_TAG_TITLE_SORTNAME         "title-sortname"
#define LOMO_TAG_ARTIST                 "artist"
#define LOMO_TAG_ARTIST_SORTNAME        "musicbrainz-sortname"
#define LOMO_TAG_ALBUM                  "album"
#define LOMO_TAG_ALBUM_SORTNAME         "album-sortname"
#define LOMO_TAG_COMPOSER               "composer"
#define LOMO_TAG_DATE                   "date"
#define LOMO_TAG_GENRE                  "genre"
#define LOMO_TAG_COMMENT                "comment"
#define LOMO_TAG_EXTENDED_COMMENT       "extended-comment"
#define LOMO_TAG_TRACK_NUMBER           "track-number"
#define LOMO_TAG_TRACK_COUNT            "track-count"
#define LOMO_TAG_ALBUM_VOLUME_NUMBER    "album-disc-number"
#define LOMO_TAG_ALBUM_VOLUME_COUNT    "album-disc-count"
#define LOMO_TAG_LOCATION               "location"
#define LOMO_TAG_DESCRIPTION            "description"
#define LOMO_TAG_VERSION                "version"
#define LOMO_TAG_ISRC                   "isrc"
#define LOMO_TAG_ORGANIZATION           "organization"
#define LOMO_TAG_COPYRIGHT              "copyright"
#define LOMO_TAG_COPYRIGHT_URI          "copyright-uri"
#define LOMO_TAG_CONTACT                "contact"
#define LOMO_TAG_LICENSE                "license"
#define LOMO_TAG_LICENSE_URI            "license-uri"
#define LOMO_TAG_PERFORMER              "performer"
#define LOMO_TAG_DURATION               "duration"
#define LOMO_TAG_CODEC                  "codec"
#define LOMO_TAG_VIDEO_CODEC            "video-codec"
#define LOMO_TAG_AUDIO_CODEC            "audio-codec"
#define LOMO_TAG_BITRATE                "bitrate"
#define LOMO_TAG_NOMINAL_BITRATE        "nominal-bitrate"
#define LOMO_TAG_MINIMUM_BITRATE        "minimum-bitrate"
#define LOMO_TAG_MAXIMUM_BITRATE        "maximum-bitrate"
#define LOMO_TAG_SERIAL                 "serial"
#define LOMO_TAG_ENCODER                "encoder"
#define LOMO_TAG_ENCODER_VERSION        "encoder-version"
#define LOMO_TAG_TRACK_GAIN             "replaygain-track-gain"
#define LOMO_TAG_TRACK_PEAK             "replaygain-track-peak"
#define LOMO_TAG_ALBUM_GAIN             "replaygain-album-gain"
#define LOMO_TAG_ALBUM_PEAK             "replaygain-album-peak"
#define LOMO_TAG_REFERENCE_LEVEL        "replaygain-reference-level"
#define LOMO_TAG_LANGUAGE_CODE          "language-code"
#define LOMO_TAG_IMAGE                  "image"
#define LOMO_TAG_PREVIEW_IMAGE          "preview-image"
#define LOMO_TAG_ATTACHMENT             "attachment"
#define LOMO_TAG_BEATS_PER_MINUTE       "beats-per-minute"
#define LOMO_TAG_KEYWORDS               "keywords"
#define LOMO_TAG_GEO_LOCATION_NAME               "geo-location-name"
#define LOMO_TAG_GEO_LOCATION_LATITUDE               "geo-location-latitude"
#define LOMO_TAG_GEO_LOCATION_LONGITUDE               "geo-location-longitude"

G_END_DECLS

#endif // _LOMO_STREAM_H
