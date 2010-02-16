/*
 * lomo/lomo-stream.h
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

#ifndef _LOMO_STREAM_H
#define _LOMO_STREAM_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * LomoTag:
 *
 * A string representing a tag
 */
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

/**
 * LomoStream:
 *
 * LomoStream represents a stream
 */
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

/**
 * lomo_stream_get_tag:
 * @stream: a #LomoStream
 * @tag: a #LomoTag
 *
 * Gets a tag from #LomoStream. The returned value is owned by @stream, and
 * should not be modified (Internally it uses g_object_get_data).
 *
 * Returns: A pointer to the tag value
 */
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

/**
 * LOMO_TAG_INVALID:
 *
 * LomoStream invalid tag, used for internal purposes
 */
#define LOMO_TAG_INVALID NULL

/**
 * LOMO_TAG_URI:
 *
 * LomoStream uri (this is a special tag)
 */
#define LOMO_TAG_URI "uri"

/**
 * LOMO_TAG_TITLE:
 *
 * LomoStream "title" tag
 */
#define LOMO_TAG_TITLE                  "title"

/**
 * LOMO_TAG_TITLE_SORTNAME:
 *
 * LomoStream "title-sortname" tag
 */
#define LOMO_TAG_TITLE_SORTNAME         "title-sortname"

/**
 * LOMO_TAG_ARTIST:
 *
 * LomoStream "artist" tag
 */
#define LOMO_TAG_ARTIST                 "artist"

/**
 * LOMO_TAG_ARTIST_SORTNAME:
 *
 * LomoStream "musicbrainz-sortname" tag
 */
#define LOMO_TAG_ARTIST_SORTNAME        "musicbrainz-sortname"

/**
 * LOMO_TAG_ALBUM:
 *
 * LomoStream "album" tag
 */
#define LOMO_TAG_ALBUM                  "album"

/**
 * LOMO_TAG_ALBUM_SORTNAME:
 *
 * LomoStream "album-sortname" tag
 */
#define LOMO_TAG_ALBUM_SORTNAME         "album-sortname"

/**
 * LOMO_TAG_COMPOSER:
 *
 * LomoStream "composer" tag
 */
#define LOMO_TAG_COMPOSER               "composer"

/**
 * LOMO_TAG_DATE:
 *
 * LomoStream "date" tag
 */
#define LOMO_TAG_DATE                   "date"

/**
 * LOMO_TAG_GENRE:
 *
 * LomoStream "genre" tag
 */
#define LOMO_TAG_GENRE                  "genre"

/**
 * LOMO_TAG_COMMENT:
 *
 * LomoStream "comment" tag
 */
#define LOMO_TAG_COMMENT                "comment"

/**
 * LOMO_TAG_EXTENDED_COMMENT:
 *
 * LomoStream "extended-comment" tag
 */
#define LOMO_TAG_EXTENDED_COMMENT       "extended-comment"

/**
 * LOMO_TAG_TRACK_NUMBER:
 *
 * LomoStream "track-number" tag
 */
#define LOMO_TAG_TRACK_NUMBER           "track-number"

/**
 * LOMO_TAG_TRACK_COUNT:
 *
 * LomoStream "track-count" tag
 */
#define LOMO_TAG_TRACK_COUNT            "track-count"

/**
 * LOMO_TAG_ALBUM_VOLUME_NUMBER:
 *
 * LomoStream "album-disc-number" tag
 */
#define LOMO_TAG_ALBUM_VOLUME_NUMBER    "album-disc-number"

/**
 * LOMO_TAG_ALBUM_VOLUME_COUNT:
 *
 * LomoStream "album-disc-count" tag
 */
#define LOMO_TAG_ALBUM_VOLUME_COUNT    "album-disc-count"

/**
 * LOMO_TAG_LOCATION:
 *
 * LomoStream "location" tag
 */
#define LOMO_TAG_LOCATION               "location"

/**
 * LOMO_TAG_DESCRIPTION:
 *
 * LomoStream "description" tag
 */
#define LOMO_TAG_DESCRIPTION            "description"

/**
 * LOMO_TAG_VERSION:
 *
 * LomoStream "version" tag
 */
#define LOMO_TAG_VERSION                "version"

/**
 * LOMO_TAG_ISRC:
 *
 * LomoStream "isrc" tag
 */
#define LOMO_TAG_ISRC                   "isrc"

/**
 * LOMO_TAG_ORGANIZATION:
 *
 * LomoStream "organization" tag
 */
#define LOMO_TAG_ORGANIZATION           "organization"

/**
 * LOMO_TAG_COPYRIGHT:
 *
 * LomoStream "copyright" tag
 */
#define LOMO_TAG_COPYRIGHT              "copyright"

/**
 * LOMO_TAG_COPYRIGHT_URI:
 *
 * LomoStream "copyright-uri" tag
 */
#define LOMO_TAG_COPYRIGHT_URI          "copyright-uri"

/**
 * LOMO_TAG_CONTACT:
 *
 * LomoStream "contact" tag
 */
#define LOMO_TAG_CONTACT                "contact"

/**
 * LOMO_TAG_LICENSE:
 *
 * LomoStream "license" tag
 */
#define LOMO_TAG_LICENSE                "license"

/**
 * LOMO_TAG_LICENSE_URI:
 *
 * LomoStream "license-uri" tag
 */
#define LOMO_TAG_LICENSE_URI            "license-uri"

/**
 * LOMO_TAG_PERFORMER:
 *
 * LomoStream "performer" tag
 */
#define LOMO_TAG_PERFORMER              "performer"

/**
 * LOMO_TAG_DURATION:
 *
 * LomoStream "duration" tag
 */
#define LOMO_TAG_DURATION               "duration"

/**
 * LOMO_TAG_CODEC:
 *
 * LomoStream "codec" tag
 */
#define LOMO_TAG_CODEC                  "codec"

/**
 * LOMO_TAG_VIDEO_CODEC:
 *
 * LomoStream "video-codec" tag
 */
#define LOMO_TAG_VIDEO_CODEC            "video-codec"

/**
 * LOMO_TAG_AUDIO_CODEC:
 *
 * LomoStream "audio-codec" tag
 */
#define LOMO_TAG_AUDIO_CODEC            "audio-codec"

/**
 * LOMO_TAG_BITRATE:
 *
 * LomoStream "bitrate" tag
 */
#define LOMO_TAG_BITRATE                "bitrate"

/**
 * LOMO_TAG_NOMINAL_BITRATE:
 *
 * LomoStream "nominal-bitrate" tag
 */
#define LOMO_TAG_NOMINAL_BITRATE        "nominal-bitrate"

/**
 * LOMO_TAG_MINIMUM_BITRATE:
 *
 * LomoStream "minimum-bitrate" tag
 */
#define LOMO_TAG_MINIMUM_BITRATE        "minimum-bitrate"

/**
 * LOMO_TAG_MAXIMUM_BITRATE:
 *
 * LomoStream "maximum-bitrate" tag
 */
#define LOMO_TAG_MAXIMUM_BITRATE        "maximum-bitrate"

/**
 * LOMO_TAG_SERIAL:
 *
 * LomoStream "serial" tag
 */
#define LOMO_TAG_SERIAL                 "serial"

/**
 * LOMO_TAG_ENCODER:
 *
 * LomoStream "encoder" tag
 */
#define LOMO_TAG_ENCODER                "encoder"

/**
 * LOMO_TAG_ENCODER_VERSION:
 *
 * LomoStream "encoder-version" tag
 */
#define LOMO_TAG_ENCODER_VERSION        "encoder-version"

/**
 * LOMO_TAG_TRACK_GAIN:
 *
 * LomoStream "replaygain-track-gain" tag
 */
#define LOMO_TAG_TRACK_GAIN             "replaygain-track-gain"

/**
 * LOMO_TAG_TRACK_PEAK:
 *
 * LomoStream "replaygain-track-peak" tag
 */
#define LOMO_TAG_TRACK_PEAK             "replaygain-track-peak"

/**
 * LOMO_TAG_ALBUM_GAIN:
 *
 * LomoStream "replaygain-album-gain" tag
 */
#define LOMO_TAG_ALBUM_GAIN             "replaygain-album-gain"

/**
 * LOMO_TAG_ALBUM_PEAK:
 *
 * LomoStream "replaygain-album-peak" tag
 */
#define LOMO_TAG_ALBUM_PEAK             "replaygain-album-peak"

/**
 * LOMO_TAG_REFERENCE_LEVEL:
 *
 * LomoStream "replaygain-reference-level" tag
 */
#define LOMO_TAG_REFERENCE_LEVEL        "replaygain-reference-level"

/**
 * LOMO_TAG_LANGUAGE_CODE:
 *
 * LomoStream "language-code" tag
 */
#define LOMO_TAG_LANGUAGE_CODE          "language-code"

/**
 * LOMO_TAG_IMAGE:
 *
 * LomoStream "image" tag
 */
#define LOMO_TAG_IMAGE                  "image"

/**
 * LOMO_TAG_PREVIEW_IMAGE:
 *
 * LomoStream "preview-image" tag
 */
#define LOMO_TAG_PREVIEW_IMAGE          "preview-image"

/**
 * LOMO_TAG_ATTACHMENT:
 *
 * LomoStream "attachment" tag
 */
#define LOMO_TAG_ATTACHMENT             "attachment"

/**
 * LOMO_TAG_BEATS_PER_MINUTE:
 *
 * LomoStream "beats-per-minute" tag
 */
#define LOMO_TAG_BEATS_PER_MINUTE       "beats-per-minute"

/**
 * LOMO_TAG_KEYWORDS:
 *
 * LomoStream "keywords" tag
 */
#define LOMO_TAG_KEYWORDS               "keywords"

/**
 * LOMO_TAG_GEO_LOCATION_NAME:
 *
 * LomoStream "geo-location-name" tag
 */
#define LOMO_TAG_GEO_LOCATION_NAME               "geo-location-name"

/**
 * LOMO_TAG_GEO_LOCATION_LATITUDE:
 *
 * LomoStream "geo-location-latitude" tag
 */
#define LOMO_TAG_GEO_LOCATION_LATITUDE               "geo-location-latitude"

/**
 * LOMO_TAG_GEO_LOCATION_LONGITUDE:
 *
 * LomoStream "geo-location-longitude" tag
 */
#define LOMO_TAG_GEO_LOCATION_LONGITUDE               "geo-location-longitude"


G_END_DECLS

#endif // _LOMO_STREAM_H
